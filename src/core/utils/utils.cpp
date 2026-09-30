#include "utils.hpp"
#include <vector>

namespace Utils {
long long GetTimeOfChildCategories(Database& db, int id,
                                   const std::string& start_utc,
                                   const std::string& end_utc) {
    std::vector<int> ids;
    if (!db.GetChildCategories(ids, id)) return 0;

    // 自分自身の分も加算する(フォルダなら記録が無いので 0)
    long long total = db.GetTotalTime(id, start_utc, end_utc);
    for (int cid : ids) {
        total += db.GetTotalTime(cid, start_utc, end_utc);
    }
    return total;
}
}
