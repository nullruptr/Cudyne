#include "database.hpp"
#include <iostream>

namespace {
// SQLite の datetime() が、同じ文字列に正規化できる = 実在する "YYYY-MM-DD HH:MM:SS"
bool IsUtcString(soci::session& sql, const std::string& text) {
    if (text.size() != 19) return false;
    std::string normalized;
    soci::indicator ind = soci::i_null;
    sql << "SELECT datetime(:t)", soci::use(text, "t"), soci::into(normalized, ind);
    return ind == soci::i_ok && normalized == text;
}
}

std::vector<Database::PlanItem> Database::GetPlans(const std::string& from_utc, const std::string& to_utc, int category_id) {
    std::vector<PlanItem> results;
    if (sql.get_backend() == nullptr) return results;

    try {
        // 上限がなければ、十分に先の時刻で代用する
        const std::string upper_utc = to_utc.empty() ? std::string("9999-12-31 23:59:59") : to_utc;

        // カテゴリの絞り込み。フォルダを指定したときは、配下のタスクも含める (int だけを連結するので安全)
        std::string category_filter;
        if (category_id != -1) {
            std::vector<int> ids{category_id};
            std::vector<int> children;
            if (GetChildCategories(children, category_id)) ids.insert(ids.end(), children.begin(), children.end());

            category_filter = " AND category_id IN (";
            for (size_t i = 0; i < ids.size(); ++i) {
                if (i > 0) category_filter += ",";
                category_filter += std::to_string(ids[i]);
            }
            category_filter += ")";
        }

        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT id, goal_id, category_id, start_time, end_time, memo FROM plan "
            "WHERE end_time > :from_utc AND start_time < :to_utc" + category_filter + " ORDER BY start_time, id",
            soci::use(from_utc, "from_utc"), soci::use(upper_utc, "to_utc"));

        for (const soci::row& row : rs) {
            PlanItem p;
            p.id          = (int)row.get<long long>(0);
            p.goal_id     = row.get_indicator(1) == soci::i_null ? -1 : (int)row.get<long long>(1);
            p.category_id = (int)row.get<long long>(2);
            p.start_time  = row.get<std::string>(3);
            p.end_time    = row.get<std::string>(4);
            p.memo        = row.get_indicator(5) == soci::i_null ? "" : row.get<std::string>(5);
            results.push_back(p);
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "GetPlans Error: " << e.what() << std::endl;
    }
    return results;
}

std::string Database::ValidatePlanItem(const PlanItem& item) {
    if (sql.get_backend() == nullptr) return "Database is not connected";

    try {
        if (!IsUtcString(sql, item.start_time)) return "Invalid start time";
        if (!IsUtcString(sql, item.end_time)) return "Invalid end time";
        // "YYYY-MM-DD HH:MM:SS" なので、文字列の比較で前後を判定できる
        if (item.end_time <= item.start_time) return "End time must be after start time";

        // カテゴリは、存在し、非表示でなく、フォルダでもないこと (実績を記録できるのはタスクだけ)
        int exists = 0;
        sql << "SELECT COUNT(*) FROM categories WHERE id = :id",
            soci::use(item.category_id, "id"), soci::into(exists);
        if (exists == 0) return "Category does not exist";

        // is_folder / is_hidden は NULL のことがある (NULL は 0 として扱う)
        int is_folder = 0, is_hidden = 0;
        soci::indicator folder_ind = soci::i_null, hidden_ind = soci::i_null;
        sql << "SELECT is_folder, is_hidden FROM categories WHERE id = :id",
            soci::use(item.category_id, "id"), soci::into(is_folder, folder_ind), soci::into(is_hidden, hidden_ind);
        if (folder_ind == soci::i_ok && is_folder == 1) return "Category is a folder (select a task)";
        if (hidden_ind == soci::i_ok && is_hidden == 1) return "Category is hidden";
    } catch (const soci::soci_error& e) {
        std::cerr << "ValidatePlanItem Error: " << e.what() << std::endl;
        return std::string("Database error: ") + e.what();
    }
    return "";
}

bool Database::ApplyPlanChanges(const std::vector<PlanItem>& created, const std::vector<PlanItem>& updated,
                                const std::vector<int>& deleted, std::string& error, std::vector<int>* new_ids) {
    error.clear();
    if (new_ids) new_ids->clear();
    if (sql.get_backend() == nullptr) {
        error = "Database is not connected";
        return false;
    }

    // 先にすべて検証する。1 件でも問題があれば、何も書かない
    for (const PlanItem& p : created) {
        const std::string e = ValidatePlanItem(p);
        if (!e.empty()) { error = e; return false; }
    }
    for (const PlanItem& p : updated) {
        const std::string e = ValidatePlanItem(p);
        if (!e.empty()) { error = e; return false; }
    }

    try {
        soci::transaction tr(sql); // commit しないまま抜けると、自動で巻き戻る

        std::vector<int> ids;
        for (const PlanItem& p : created) {
            soci::indicator goal_ind = (p.goal_id == -1) ? soci::i_null : soci::i_ok;
            sql << "INSERT INTO plan (goal_id, category_id, start_time, end_time, memo) "
                   "VALUES (:goal_id, :category_id, :start_time, :end_time, :memo)",
                soci::use(p.goal_id, goal_ind, "goal_id"),
                soci::use(p.category_id, "category_id"),
                soci::use(p.start_time, "start_time"),
                soci::use(p.end_time, "end_time"),
                soci::use(p.memo, "memo");

            long long new_id = 0;
            sql << "SELECT last_insert_rowid()", soci::into(new_id);
            ids.push_back((int)new_id);
        }

        // goal_id は更新しない (表に出していない値を、上書きで消さないため)
        for (const PlanItem& p : updated) {
            sql << "UPDATE plan SET category_id = :category_id, start_time = :start_time, "
                   "end_time = :end_time, memo = :memo WHERE id = :id",
                soci::use(p.category_id, "category_id"),
                soci::use(p.start_time, "start_time"),
                soci::use(p.end_time, "end_time"),
                soci::use(p.memo, "memo"),
                soci::use(p.id, "id");
        }

        for (int id : deleted) {
            sql << "DELETE FROM plan WHERE id = :id", soci::use(id, "id");
        }

        // 反映後の状態で、追加・更新した予定が、同じカテゴリの他の予定と時間が重ならないこと。
        // (同じ時刻の予定も重なり。端が接しているだけならよい。表に出ていない予定とも照合できるよう、DB で確かめる)
        // 重なりがあれば、commit せずに抜けるので、ここまでの変更はすべて巻き戻る
        auto find_overlap = [&](const PlanItem& p, int self_id) {
            int other = -1;
            soci::indicator ind = soci::i_null;
            sql << "SELECT id FROM plan WHERE category_id = :category_id AND id <> :self_id "
                   "AND start_time < :end_time AND end_time > :start_time LIMIT 1",
                soci::use(p.category_id, "category_id"), soci::use(self_id, "self_id"),
                soci::use(p.end_time, "end_time"), soci::use(p.start_time, "start_time"),
                soci::into(other, ind);
            return ind == soci::i_ok ? other : -1;
        };
        for (size_t i = 0; i < created.size(); ++i) {
            const int other = find_overlap(created[i], ids[i]);
            if (other != -1) {
                error = "Plan overlaps with plan ID " + std::to_string(other) + " (same category and time)";
                return false;
            }
        }
        for (const PlanItem& p : updated) {
            const int other = find_overlap(p, p.id);
            if (other != -1) {
                error = "Plan ID " + std::to_string(p.id) + " overlaps with plan ID " + std::to_string(other) + " (same category and time)";
                return false;
            }
        }

        tr.commit();
        if (new_ids) *new_ids = ids;
    } catch (const soci::soci_error& e) {
        std::cerr << "ApplyPlanChanges Error: " << e.what() << std::endl;
        error = std::string("Database error: ") + e.what();
        return false;
    }
    return true;
}
