#pragma once
// goal の対象 (Category / ToDo) の実績を、DB から集計する
#include <algorithm>
#include <ctime>
#include <string>
#include <vector>
#include "core/db/database.hpp"
#include "core/utils/utils.hpp"

namespace GoalAchieved {

// "%Y-%m-%d %H:%M:%S" (UTC)
inline std::string ToUtcString(long long epoch) {
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm = {};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

struct Total {
    long long seconds = 0;
    bool recording = false; // 対象に記録中の record がある (実績が時間とともに増える)
};

// [from, to) の実績(秒)。Category はサブツリーを合算する。記録中の record は now までを含める
inline Total Aggregate(Database& db, const Database::Goal& g, long long from, long long to, long long now) {
    Total result;
    if (to <= from) return result;

    const std::string from_utc = ToUtcString(from);
    const std::string to_utc = ToUtcString(to);

    long long total = 0;
    std::vector<int> category_ids;
    if (g.category_id != -1) {
        total = Utils::GetTimeOfChildCategories(db, g.category_id, from_utc, to_utc);
        category_ids.push_back(g.category_id);
        std::vector<int> children;
        if (db.GetChildCategories(children, g.category_id)) {
            category_ids.insert(category_ids.end(), children.begin(), children.end());
        }
    } else if (g.todo_id != -1) {
        total = db.GetTotalTimeByTodo(g.todo_id, from_utc, to_utc);
    } else {
        return result;
    }

    // 記録中の record は time_end が空なので、上の集計に含まれない
    for (const Database::Record& r : db.GetUnfinishedRecords()) {
        const bool match = g.category_id != -1
            ? std::find(category_ids.begin(), category_ids.end(), r.category_id) != category_ids.end()
            : r.todo_id == g.todo_id;
        if (!match) continue;

        result.recording = true;
        const long long s = std::max(r.time_begin, from);
        const long long e = std::min(now, to);
        if (e > s) total += e - s;
    }
    result.seconds = total;
    return result;
}

inline long long Seconds(Database& db, const Database::Goal& g, long long from, long long to, long long now) {
    return Aggregate(db, g, from, to, now).seconds;
}

} // namespace GoalAchieved
