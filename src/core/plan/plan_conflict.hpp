#pragma once
// 予定の重なり (同じカテゴリで、時間が重なる予定) の検出。仕様は plan.md。DB と wxWidgets には依存しない
#include <algorithm>
#include <utility>
#include <vector>

namespace PlanConflict {

struct Entry {
    int group = 0;          // 重なりを調べる単位 (カテゴリ ID)
    long long start = 0;    // epoch 秒
    long long end = 0;
    int index = 0;          // 呼び出し側で、元の行を特定するための番号
};

// 同じ group で、時間が重なる予定の組 (index の組) を返す。同じ時刻の予定も重なりとする。
// 端が接しているだけ (一方の end == 他方の start) は、重ならない。
inline std::vector<std::pair<int, int>> Find(std::vector<Entry> entries) {
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.group != b.group) return a.group < b.group;
        if (a.start != b.start) return a.start < b.start;
        if (a.end != b.end) return a.end < b.end;
        return a.index < b.index;
    });

    std::vector<std::pair<int, int>> pairs;
    std::vector<const Entry*> active; // いま見ている開始時刻の時点で、まだ終わっていない予定 (同じ group)
    int current_group = 0;
    bool has_group = false;

    for (const Entry& e : entries) {
        if (!has_group || e.group != current_group) {
            active.clear();
            current_group = e.group;
            has_group = true;
        }
        // 終わった予定を外す (end <= start なら、重ならない)
        active.erase(std::remove_if(active.begin(), active.end(),
                                    [&](const Entry* a) { return a->end <= e.start; }),
                     active.end());
        for (const Entry* a : active) pairs.emplace_back(a->index, e.index);
        active.push_back(&e);
    }
    return pairs;
}

} // namespace PlanConflict
