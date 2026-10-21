#pragma once
// 予定で Goal を達成できるかの判定。仕様は plan.md。DB と wxWidgets には依存しない。時刻はすべて epoch 秒(UTC)
#include <algorithm>
#include <vector>

namespace PlanCheck {

constexpr long long kNoDeadline = -1;

enum class Verdict {
    Achieved,    // すでに目標に届いている
    Covered,     // 残りの時間が、これからの予定でまかなえる
    Short,       // 予定が足りない。ただし、窓の残り時間を使えば間に合う
    Impossible,  // 窓の残り時間を、休まず使っても間に合わない
    Overdue,     // 期限(Rolling のサイクルの n 日)を過ぎている
};

struct Span {
    long long start = 0;
    long long end = 0;
};

struct Result {
    Verdict verdict = Verdict::Achieved;
    long long remaining = 0;   // 目標 - 実績 (0 以上)
    long long planned = 0;     // 窓のうち、これから先に入っている予定の合計
    long long shortfall = 0;   // 予定が足りない分 (Covered / Achieved は 0)
    long long time_left = kNoDeadline; // 窓の残り時間 (期限がなければ kNoDeadline)
};

// [a0, a1) と [b0, b1) が重なる秒数
inline long long Overlap(long long a0, long long a1, long long b0, long long b1) {
    return std::max(0LL, std::min(a1, b1) - std::max(a0, b0));
}

// 1 つの窓の判定。
// - target: その窓の目標(秒)。例外設定があれば、差し替え後の値
// - achieved: 実績(秒)。記録中の分を含む
// - window_start / deadline: 窓の始点と期限 (期限がなければ kNoDeadline)
// - plans: その Goal の対象 (Category 以下) の予定だけを渡す
// 数えるのは、今以降の予定だけ。すでに過ぎた予定は、実績(achieved)に現れるので、二重に数えない。
inline Result Judge(long long target, long long achieved, long long now, long long window_start,
                    long long deadline, const std::vector<Span>& plans) {
    Result r;
    r.remaining = std::max(0LL, target - achieved);
    if (r.remaining <= 0) {
        r.verdict = Verdict::Achieved;
        return r;
    }

    const bool has_deadline = deadline != kNoDeadline;
    const long long from = std::max(now, window_start);
    const long long far_future = from + 100LL * 365 * 24 * 3600; // 期限がなければ、十分先まで
    const long long to = has_deadline ? deadline : far_future;

    if (has_deadline && deadline <= now) {
        // 期限は過ぎている。今から先の予定は(期限に関係なく)参考に数えるが、判定は「超過」
        r.verdict = Verdict::Overdue;
        for (const Span& p : plans) r.planned += Overlap(p.start, p.end, now, far_future);
        r.shortfall = std::max(0LL, r.remaining - r.planned);
        return r;
    }

    for (const Span& p : plans) r.planned += Overlap(p.start, p.end, from, to);
    if (has_deadline) r.time_left = to - from;

    if (r.planned >= r.remaining) {
        r.verdict = Verdict::Covered;
        return r;
    }

    r.shortfall = r.remaining - r.planned;
    r.verdict = (has_deadline && r.remaining > r.time_left) ? Verdict::Impossible : Verdict::Short;
    return r;
}

inline const char* VerdictName(Verdict v) {
    switch (v) {
    case Verdict::Achieved:   return "Achieved";
    case Verdict::Covered:    return "Covered";
    case Verdict::Short:      return "Short";
    case Verdict::Impossible: return "Impossible";
    case Verdict::Overdue:    return "Overdue";
    }
    return "";
}

} // namespace PlanCheck
