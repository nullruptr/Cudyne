#pragma once
// Goal の評価。仕様は goal.md。時刻は epoch 秒(UTC)、窓の境界はローカル時刻で決める
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

namespace GoalEval {

// DB の goal.period_type と同じ値
enum class PeriodType : int {
    Daily      = 0,
    Weekly     = 1,
    Monthly    = 2,
    EveryNDays = 3,
    Undefined  = 4,
    Rolling    = 5,               // 達成時刻を起点に、n 日以内を繰り返す
};

enum class State {
    Disabled,
    EndedAchieved,
    EndedMissed,
    Achieved,
    NotStarted,
    Missed,
    Urgent,
    Behind,
    Comfortable,
};

constexpr long long kNone = -1;

// 遅れ(Behind)の許容幅(pt)。窓の開始直後に、経過率が小さいだけで警告が出ないようにする
constexpr double kBehindMarginPt = 5.0;

struct GoalSpec {
    PeriodType period_type = PeriodType::Daily;
    int period_n = 1;             // EveryNDays / Rolling のとき
    long long target_seconds = 0;
    long long start_epoch = 0;
    long long end_epoch = kNone;
    bool is_active = true;
    long long cycle_start_epoch = kNone; // Rolling のみ。現在のサイクルの開始。kNone なら start_epoch
};

struct Window {
    bool valid = false;           // EveryNDays の最初の窓より前は false
    long long start = 0;
    long long end = kNone;        // 実績を集計する終点。End で打ち切り済み
    long long deadline = kNone;   // 猶予と経過率の基準。end と同じだが、Rolling は期限後も窓が閉じないため別
};

struct Result {
    State state = State::Comfortable;
    Window window;
    long long achieved = 0;
    long long target = 0;
    long long remaining = 0;
    bool has_slack = false;       // 窓の期限がなければ false
    long long slack = 0;          // 負になり得る
    bool has_pace = false;        // 経過率と達成率を計算できるか (期限があり、目標が 0 でない)
    double elapsed_pct = 0;       // 経過率 (0〜100)
    double achieved_pct = 0;      // 達成率 (100 を超え得る)
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
    case PeriodType::Rolling: {
        const long long s = spec.cycle_start_epoch != kNone ? spec.cycle_start_epoch : spec.start_epoch;
        if (at < s) {
            w.valid = false;
            break;
        }
        // n 日は暦日で数える。期限を過ぎても窓は閉じない (達成して初めて次のサイクルへ)
        std::tm tm = LocalTm(s);
        tm.tm_mday += std::max(1, spec.period_n);
        w.start = s;
        w.end = kNone;
        w.deadline = FromLocalTm(tm);
        break;
    }
    case PeriodType::Undefined:
    default:
        w.start = spec.start_epoch;
        w.end = kNone;
        break;
    }

    if (spec.period_type != PeriodType::Rolling) w.deadline = w.end;

    if (w.valid && has_end) {
        if (w.end == kNone || w.end > spec.end_epoch) w.end = spec.end_epoch;
        if (w.deadline == kNone || w.deadline > spec.end_epoch) w.deadline = spec.end_epoch;
    }
    return w;
}

// 目標の例外設定を指す、窓のキー = 窓の始点のローカル暦日 ("YYYY-MM-DD")。
// タイムゾーンを変えても、同じ日付が同じ窓を指す。Rolling / Undefined は例外設定の対象外
inline std::string WindowKey(long long window_start) {
    const std::tm tm = detail::LocalTm(window_start);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

// 例外設定に対応している Period Type か
inline bool SupportsOverride(PeriodType t) {
    return t != PeriodType::Rolling && t != PeriodType::Undefined;
}

// "YYYY-MM-DD" (ローカル暦日) の 0 時の epoch。不正な形式は kNone
inline long long WindowKeyToEpoch(const std::string& key) {
    // "YYYY-MM-DD" 固定の形式
    if (key.size() != 10 || key[4] != '-' || key[7] != '-') return kNone;
    for (size_t i = 0; i < key.size(); ++i) {
        if (i != 4 && i != 7 && (key[i] < '0' || key[i] > '9')) return kNone;
    }
    std::tm tm = {};
    tm.tm_year = std::atoi(key.substr(0, 4).c_str()) - 1900;
    tm.tm_mon = std::atoi(key.substr(5, 2).c_str()) - 1;
    tm.tm_mday = std::atoi(key.substr(8, 2).c_str());
    return detail::FromLocalTm(tm);
}

// 終了済みの record の区間 (epoch 秒)
struct Span {
    long long begin = 0;
    long long end = 0;
};

// Rolling の達成時刻 T。cycle_start 以降の実績が target に達した record の終了時刻を返す (届かなければ kNone)。
// 超過分は捨てるので、T は record の途中ではなく終了時刻。cycle_start をまたぐ record は重なる部分だけを数える。
// spans は begin 昇順。
inline long long FindAchievedAt(const std::vector<Span>& spans, long long cycle_start, long long target) {
    if (target <= 0) return kNone;
    long long total = 0;
    for (const Span& s : spans) {
        if (s.end <= cycle_start) continue;
        total += s.end - std::max(s.begin, cycle_start);
        if (total >= target) return s.end;
    }
    return kNone;
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
    if (w.valid && w.deadline != kNone) {
        r.has_slack = true;
        r.slack = w.deadline - now - r.remaining;

        // 経過率 = (今 - 窓の始点) / (期限 - 窓の始点)。期限を過ぎたら 100
        if (r.target > 0 && w.deadline > w.start) {
            r.has_pace = true;
            const double span = static_cast<double>(w.deadline - w.start);
            r.elapsed_pct = std::min(100.0, std::max(0.0, (now - w.start) / span * 100.0));
            r.achieved_pct = static_cast<double>(r.achieved) / static_cast<double>(r.target) * 100.0;
        }
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
    } else if (r.has_pace && r.achieved_pct < r.elapsed_pct - kBehindMarginPt) {
        r.state = State::Behind;
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
    case State::Behind:        return "Behind";
    case State::Comfortable:   return "Comfortable";
    }
    return "";
}

} // namespace GoalEval
