#pragma once
// DB の goal を、今の時点で評価する (Goal 一覧と Detail で共通に使う)
#include <algorithm>
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

struct Evaluated {
    GoalEval::GoalSpec spec;
    GoalEval::Window window;
    GoalAchieved::Total total;
    GoalEval::Result result;
};

// now 時点の窓の実績を集計して評価する
inline Evaluated EvaluateNow(Database& db, const Database::Goal& g, long long now) {
    Evaluated ev;
    ev.spec = ToSpec(g);
    ev.window = GoalEval::GetWindow(ev.spec, now);
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
