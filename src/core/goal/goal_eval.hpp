#pragma once
// Goal の評価。仕様は goal.md。時刻は epoch 秒(UTC)、窓の境界はローカル時刻で決める
#include <algorithm>
#include <ctime>

namespace GoalEval {

// DB の goal.period_type と同じ値
enum class PeriodType : int {
    Daily      = 0,
    Weekly     = 1,
    Monthly    = 2,
    EveryNDays = 3,
    Undefined  = 4,
};

enum class State {
    Disabled,
    EndedAchieved,
    EndedMissed,
    Achieved,
    NotStarted,
    Missed,
    Urgent,
    Comfortable,
};

constexpr long long kNone = -1;

struct GoalSpec {
    PeriodType period_type = PeriodType::Daily;
    int period_n = 1;             // EveryNDays のとき
    long long target_seconds = 0;
    long long start_epoch = 0;
    long long end_epoch = kNone;
    bool is_active = true;
};

struct Window {
    bool valid = false;           // EveryNDays の最初の窓より前は false
    long long start = 0;
    long long end = kNone;        // End で打ち切り済み
};

struct Result {
    State state = State::Comfortable;
    Window window;
    long long achieved = 0;
    long long target = 0;
    long long remaining = 0;
    bool has_slack = false;       // 窓の終点がなければ false
    long long slack = 0;          // 負になり得る
};

namespace detail {

constexpr long long kHour = 3600;
constexpr long long kDay = 24 * kHour;

inline std::tm LocalTm(long long epoch) {
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm = {};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm;
}

// mday / mon がはみ出していても mktime が正規化する
inline long long FromLocalTm(std::tm tm) {
    tm.tm_isdst = -1;
    return static_cast<long long>(std::mktime(&tm));
}

inline std::tm DayStartTm(long long epoch) {
    std::tm tm = LocalTm(epoch);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return tm;
}

// 暦日の通算日数 (夏時間に影響されない日数差用)
inline long long DaysFromCivil(int y, int m, int d) {
    y -= m <= 2 ? 1 : 0;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const long long yoe = y - era * 400;
    const long long mp = m > 2 ? m - 3 : m + 9;
    const long long doy = (153 * mp + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

inline long long DaysOf(const std::tm& tm) {
    return DaysFromCivil(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

inline long long FloorDiv(long long a, long long b) {
    long long q = a / b;
    if ((a % b != 0) && ((a < 0) != (b < 0))) --q;
    return q;
}

} // namespace detail

// at を含む窓。at >= End のときは、End を含む最後の窓を返す
inline Window GetWindow(const GoalSpec& spec, long long at) {
    using namespace detail;

    const bool has_end = spec.end_epoch != kNone;
    if (has_end && at >= spec.end_epoch) at = spec.end_epoch - 1;

    Window w;
    w.valid = true;

    switch (spec.period_type) {
    case PeriodType::Daily: {
        std::tm tm = DayStartTm(at);
        w.start = FromLocalTm(tm);
        tm.tm_mday += 1;
        w.end = FromLocalTm(tm);
        break;
    }
    case PeriodType::Weekly: {
        std::tm tm = DayStartTm(at);
        tm.tm_mday -= (tm.tm_wday + 6) % 7; // 月曜始まり
        w.start = FromLocalTm(tm);
        tm.tm_mday += 7;
        w.end = FromLocalTm(tm);
        break;
    }
    case PeriodType::Monthly: {
        std::tm tm = DayStartTm(at);
        tm.tm_mday = 1;
        w.start = FromLocalTm(tm);
        tm.tm_mon += 1;
        w.end = FromLocalTm(tm);
        break;
    }
    case PeriodType::EveryNDays: {
        const int n = std::max(1, spec.period_n);
        const std::tm anchor = DayStartTm(spec.start_epoch);
        const long long diff = DaysOf(LocalTm(at)) - DaysOf(anchor);
        if (diff < 0) {
            w.valid = false;
            break;
        }
        std::tm tm = anchor;
        tm.tm_mday += static_cast<int>(FloorDiv(diff, n) * n);
        w.start = FromLocalTm(tm);
        tm.tm_mday += n;
        w.end = FromLocalTm(tm);
        break;
    }
    case PeriodType::Undefined:
    default:
        w.start = spec.start_epoch;
        w.end = kNone;
        break;
    }

    if (w.valid && has_end && (w.end == kNone || w.end > spec.end_epoch)) {
        w.end = spec.end_epoch;
    }
    return w;
}

// 急ぎの閾値 = min(24h, max(1h, 残り / 2))
inline long long UrgentThreshold(long long remaining) {
    using namespace detail;
    return std::min(kDay, std::max(kHour, remaining / 2));
}

// achieved は GetWindow(spec, at) の窓内の実績(秒)
inline Result Evaluate(const GoalSpec& spec, long long at, long long now, long long achieved) {
    Result r;
    r.window = GetWindow(spec, at);
    r.target = spec.target_seconds;
    r.achieved = achieved;
    r.remaining = std::max(0LL, r.target - r.achieved);

    const Window& w = r.window;
    if (w.valid && w.end != kNone) {
        r.has_slack = true;
        r.slack = w.end - now - r.remaining;
    }

    if (!spec.is_active) {
        r.state = State::Disabled;
        return r;
    }

    // 終了済は End を含む最後の窓だけで判定
    if (spec.end_epoch != kNone && now >= spec.end_epoch) {
        r.state = r.remaining <= 0 ? State::EndedAchieved : State::EndedMissed;
        return r;
    }

    const bool not_started = now < spec.start_epoch;

    // Start 前でも、Start が今の窓に含まれるなら達成
    if (r.remaining <= 0) {
        bool start_in_window = false;
        if (not_started && w.valid) {
            start_in_window = spec.start_epoch >= w.start &&
                              (w.end == kNone || spec.start_epoch < w.end);
        }
        r.state = (!not_started || start_in_window) ? State::Achieved : State::NotStarted;
        return r;
    }

    if (not_started || !w.valid) {
        r.state = State::NotStarted;
        return r;
    }

    if (!r.has_slack) {
        r.state = State::Comfortable;
    } else if (r.slack < 0) {
        r.state = State::Missed;
    } else if (r.slack <= UrgentThreshold(r.remaining)) {
        r.state = State::Urgent;
    } else {
        r.state = State::Comfortable;
    }
    return r;
}

inline const char* StateName(State s) {
    switch (s) {
    case State::Disabled:      return "Disabled";
    case State::EndedAchieved: return "EndedAchieved";
    case State::EndedMissed:   return "EndedMissed";
    case State::Achieved:      return "Achieved";
    case State::NotStarted:    return "NotStarted";
    case State::Missed:        return "Missed";
    case State::Urgent:        return "Urgent";
    case State::Comfortable:   return "Comfortable";
    }
    return "";
}

} // namespace GoalEval
