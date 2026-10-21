#pragma once
// goal の窓を、指定した期間ぶん列挙して評価する (Detail の Goal タブで使う)。仕様は goal.md
#include <algorithm>
#include <string>
#include <vector>
#include "core/goal/goal_status.hpp"

namespace GoalReport {

struct Row {
    long long start = 0;
    long long end = GoalEval::kNone;  // 表示用の終点。Rolling は達成時刻か期限、Undefined で End なしは kNone
    long long target = 0;             // 例外設定を反映した目標(秒)
    long long normal_target = 0;
    bool overridden = false;
    long long achieved = 0;
    bool future = false;              // まだ始まっていない窓
    GoalEval::Result result;
};

// 1 つの goal あたりの上限。Daily で長い期間を指定したときに、集計の問い合わせが増えすぎないようにする
constexpr int kMaxRowsPerGoal = 400;

namespace impl {

inline bool Overlaps(long long start, long long end, long long from, long long to) {
    return start < to && (end == GoalEval::kNone || end > from);
}

// 終わった窓の判定。End による打ち切りで Evaluate が最後の窓だけを見るため、窓ごとに自前で判定する
inline GoalEval::Result PastResult(const GoalEval::Window& w, long long target, long long achieved) {
    GoalEval::Result r;
    r.window = w;
    r.target = target;
    r.achieved = achieved;
    r.remaining = std::max(0LL, target - achieved);
    r.state = r.remaining <= 0 ? GoalEval::State::Achieved : GoalEval::State::Missed;
    return r;
}

} // namespace impl

// [from, to) と重なる窓を、古い順に返す。truncated は、上限で打ち切ったとき true
inline std::vector<Row> List(Database& db, const Database::Goal& g, long long from, long long to,
                             long long now, bool* truncated = nullptr) {
    using namespace GoalEval;
    std::vector<Row> rows;
    if (truncated) *truncated = false;

    GoalSpec spec = GoalStatus::ToSpec(g);
    const std::vector<Database::GoalWindow> saved = db.GetGoalWindows(g.goal_id);

    auto make_row = [&](const Window& w, long long target) {
        Row row;
        row.start = w.start;
        row.end = w.deadline != kNone ? w.deadline : w.end;
        row.normal_target = spec.target_seconds;
        row.target = target;
        row.overridden = target != spec.target_seconds;
        row.future = w.start > now;
        return row;
    };

    // 実績: 窓の始点から、終点か今のうち早い方まで
    auto aggregate = [&](const Window& w) {
        const long long upto = w.end == kNone ? now : std::min(now, w.end);
        return GoalAchieved::Aggregate(db, g, w.start, upto, now).seconds;
    };

    if (spec.period_type == PeriodType::Rolling) {
        // 達成済みのサイクルを Start から順にたどり、最後に現在のサイクルを足す
        const long long current_start = GoalStatus::ResolveRollingCycleStart(db, g);
        const std::vector<Database::GoalWindow> chain = db.GetGoalWindows(g.goal_id);

        long long s = TimeUtils::ParseUTCStringToEpoch(g.start_time);
        for (int i = 0; i < kMaxRowsPerGoal && s != current_start; ++i) {
            const std::string key = GoalAchieved::ToUtcString(s);
            auto it = std::find_if(chain.begin(), chain.end(), [&](const Database::GoalWindow& w) {
                return w.window_key == key && !w.achieved_at.empty();
            });
            if (it == chain.end()) break;
            const long long t = TimeUtils::ParseUTCStringToEpoch(it->achieved_at);

            if (impl::Overlaps(s, t, from, to)) {
                Window w;
                w.valid = true;
                w.start = s;
                w.end = t;
                w.deadline = t;
                Row row = make_row(w, spec.target_seconds);
                row.achieved = GoalAchieved::Aggregate(db, g, s, t, now).seconds;
                row.result = impl::PastResult(w, spec.target_seconds, row.achieved);
                rows.push_back(row);
            }
            s = t;
        }

        spec.cycle_start_epoch = current_start;
        const Window w = GetWindow(spec, now);
        if (w.valid) {
            const long long shown_end = std::max(w.deadline, now); // 期限を過ぎても窓は閉じない
            if (impl::Overlaps(w.start, shown_end, from, to)) {
                Row row = make_row(w, spec.target_seconds);
                row.achieved = aggregate(w);
                row.result = Evaluate(spec, now, now, row.achieved);
                rows.push_back(row);
            }
        }
        return rows;
    }

    if (spec.period_type == PeriodType::Undefined) {
        const Window w = GetWindow(spec, now);
        if (w.valid && impl::Overlaps(w.start, w.end, from, to)) {
            Row row = make_row(w, spec.target_seconds);
            row.achieved = aggregate(w);
            row.result = Evaluate(spec, now, now, row.achieved);
            rows.push_back(row);
        }
        return rows;
    }

    // Daily / Weekly / Monthly / Every n Days: Start を含む窓から、End までを順にたどる
    long long at = std::max(from, spec.start_epoch);
    if (spec.end_epoch != kNone && at >= spec.end_epoch) return rows;

    int count = 0;
    while (true) {
        const Window w = GetWindow(spec, at);
        if (!w.valid || w.end == kNone || w.start >= to) break;
        if (spec.end_epoch != kNone && w.start >= spec.end_epoch) break;
        if (count >= kMaxRowsPerGoal) {
            if (truncated) *truncated = true;
            break;
        }

        if (impl::Overlaps(w.start, w.end, from, to)) {
            // 例外設定があれば、その窓の目標に差し替える
            long long target = spec.target_seconds;
            const std::string key = WindowKey(w.start);
            for (const Database::GoalWindow& sw : saved) {
                if (sw.window_key == key && sw.target_override != -1) {
                    target = sw.target_override;
                    break;
                }
            }

            Row row = make_row(w, target);
            row.achieved = w.start > now ? 0 : aggregate(w);

            if (w.end <= now) {
                row.result = impl::PastResult(w, target, row.achieved);
            } else {
                GoalSpec window_spec = spec;
                window_spec.target_seconds = target;
                row.result = Evaluate(window_spec, std::max(w.start, std::min(now, w.end - 1)), now, row.achieved);
            }
            rows.push_back(row);
            ++count;
        }

        if (w.end <= at) break; // 進まないときの保険
        at = w.end;
    }
    return rows;
}

} // namespace GoalReport
