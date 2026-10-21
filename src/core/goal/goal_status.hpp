#pragma once
// DB の goal を、今の時点で評価する (Goal 一覧と Detail で共通に使う)
#include <algorithm>
#include <string>
#include <vector>
#include "core/db/database.hpp"
#include "core/goal/goal_achieved.hpp"
#include "core/goal/goal_eval.hpp"
#include "core/utils/format_time.hpp"

namespace GoalStatus {

// DB の goal を評価用の GoalSpec に変換する (時刻は UTC 文字列から epoch 秒へ)
inline GoalEval::GoalSpec ToSpec(const Database::Goal& g) {
    GoalEval::GoalSpec spec;
    spec.period_type = (g.period_type >= 0 && g.period_type <= 5)
        ? static_cast<GoalEval::PeriodType>(g.period_type)
        : GoalEval::PeriodType::Daily;
    spec.period_n = g.period_n > 0 ? g.period_n : 1;
    spec.target_seconds = g.target_time;
    spec.start_epoch = TimeUtils::ParseUTCStringToEpoch(g.start_time);
    spec.end_epoch = g.end_time.empty() ? GoalEval::kNone : TimeUtils::ParseUTCStringToEpoch(g.end_time);
    spec.is_active = g.is_active == 1;
    return spec;
}

// goal の対象 (Category のサブツリー / ToDo) に紐づく、終了済みの record の区間。from 以降、begin 昇順
inline std::vector<GoalEval::Span> FinishedSpans(Database& db, const Database::Goal& g, long long from) {
    std::vector<int> category_ids;
    if (g.category_id != -1) {
        category_ids.push_back(g.category_id);
        std::vector<int> children;
        if (db.GetChildCategories(children, g.category_id)) {
            category_ids.insert(category_ids.end(), children.begin(), children.end());
        }
    }

    std::vector<GoalEval::Span> spans;
    for (const Database::Record& r : db.GetRecordList(GoalAchieved::ToUtcString(from), "9999-12-31 23:59:59")) {
        const bool match = g.category_id != -1
            ? std::find(category_ids.begin(), category_ids.end(), r.category_id) != category_ids.end()
            : (g.todo_id != -1 && r.todo_id == g.todo_id);
        if (match && r.time_end > r.time_begin) spans.push_back({r.time_begin, r.time_end});
    }
    return spans;
}

// Rolling の、現在のサイクルの開始時刻。
// 保存済みのサイクル(goal_window.achieved_at)をたどり、まだ保存されていない達成があれば確定して保存する。
// 記録中の record は数えない (終了して初めて達成時刻 T が決まる)。
inline long long ResolveRollingCycleStart(Database& db, const Database::Goal& g) {
    constexpr int kMaxCycles = 1000; // record の編集などで異常に多くなっても止まるように
    long long start = TimeUtils::ParseUTCStringToEpoch(g.start_time);
    if (g.target_time <= 0) return start;

    // 保存済みの達成を、Start から順にたどる
    const std::vector<Database::GoalWindow> windows = db.GetGoalWindows(g.goal_id);
    for (int i = 0; i < kMaxCycles; ++i) {
        const std::string key = GoalAchieved::ToUtcString(start);
        auto it = std::find_if(windows.begin(), windows.end(), [&](const Database::GoalWindow& w) {
            return w.window_key == key && !w.achieved_at.empty();
        });
        if (it == windows.end()) break;
        start = TimeUtils::ParseUTCStringToEpoch(it->achieved_at);
    }

    // 保存されていない達成を確定する
    const std::vector<GoalEval::Span> spans = FinishedSpans(db, g, start);
    for (int i = 0; i < kMaxCycles; ++i) {
        const long long t = GoalEval::FindAchievedAt(spans, start, g.target_time);
        if (t == GoalEval::kNone) break;
        db.SetGoalWindowAchieved(g.goal_id, GoalAchieved::ToUtcString(start), GoalAchieved::ToUtcString(t));
        start = t;
    }
    return start;
}

struct Evaluated {
    GoalEval::GoalSpec spec;
    GoalEval::Window window;
    GoalAchieved::Total total;
    GoalEval::Result result;
    bool overridden = false;       // この窓の目標が、例外設定で差し替えられている
    long long normal_target = 0;   // 例外設定がないときの目標(秒)
};

// now 時点の窓の実績を集計して評価する
inline Evaluated EvaluateNow(Database& db, const Database::Goal& g, long long now) {
    Evaluated ev;
    ev.spec = ToSpec(g);
    if (ev.spec.period_type == GoalEval::PeriodType::Rolling) {
        ev.spec.cycle_start_epoch = ResolveRollingCycleStart(db, g);
    }
    ev.window = GoalEval::GetWindow(ev.spec, now);
    ev.normal_target = ev.spec.target_seconds;

    // この窓だけの目標があれば差し替える (集計する実績は変わらない)
    if (ev.window.valid && GoalEval::SupportsOverride(ev.spec.period_type)) {
        const std::string key = GoalEval::WindowKey(ev.window.start);
        for (const Database::GoalWindow& w : db.GetGoalWindows(g.goal_id)) {
            if (w.window_key == key && w.target_override != -1) {
                ev.spec.target_seconds = w.target_override;
                ev.overridden = true;
                break;
            }
        }
    }

    if (ev.window.valid) {
        const long long to = ev.window.end == GoalEval::kNone ? now : std::min(now, ev.window.end);
        ev.total = GoalAchieved::Aggregate(db, g, ev.window.start, to, now);
    }
    ev.result = GoalEval::Evaluate(ev.spec, now, now, ev.total.seconds);
    return ev;
}

// 件数の集計や、一覧の Condition で使う分類
enum class Group { Achieved, Unachieved, Invalid };

inline Group GroupOf(GoalEval::State s) {
    using GoalEval::State;
    switch (s) {
    case State::Achieved:
    case State::EndedAchieved:
        return Group::Achieved;
    case State::Comfortable:
    case State::Urgent:
    case State::Behind:
    case State::Missed:
    case State::EndedMissed:
        return Group::Unachieved;
    case State::NotStarted:
    case State::Disabled:
    default:
        return Group::Invalid;
    }
}

} // namespace GoalStatus
