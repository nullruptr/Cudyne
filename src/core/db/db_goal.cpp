#include "database.hpp"
#include <iostream>

bool Database::InsertGoal(const Goal& goal) {
    if (sql.get_backend() == nullptr) return false;

    try {
        // -1 / 空文字は「未設定」を表すので、DB には NULL として保存する
        soci::indicator category_ind = (goal.category_id == -1) ? soci::i_null : soci::i_ok;
        soci::indicator todo_ind     = (goal.todo_id == -1)     ? soci::i_null : soci::i_ok;
        soci::indicator period_n_ind = (goal.period_n == -1)    ? soci::i_null : soci::i_ok;
        soci::indicator end_ind      = goal.end_time.empty()    ? soci::i_null : soci::i_ok;

        sql << "INSERT INTO goal (category_id, todo_id, goal_name, period_type, period_n, target_time, start_time, end_time, is_active, memo) "
               "VALUES (:category_id, :todo_id, :goal_name, :period_type, :period_n, :target_time, :start_time, :end_time, :is_active, :memo)",
            soci::use(goal.category_id, category_ind, "category_id"),
            soci::use(goal.todo_id, todo_ind, "todo_id"),
            soci::use(goal.goal_name, "goal_name"),
            soci::use(goal.period_type, "period_type"),
            soci::use(goal.period_n, period_n_ind, "period_n"),
            soci::use(goal.target_time, "target_time"),
            soci::use(goal.start_time, "start_time"),
            soci::use(goal.end_time, end_ind, "end_time"),
            soci::use(goal.is_active, "is_active"),
            soci::use(goal.memo, "memo");
    } catch (const soci::soci_error& e) {
        std::cerr << "InsertGoal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::UpdateGoal(const Goal& goal) {
    if (sql.get_backend() == nullptr) return false;

    try {
        // 窓の位置が変わる変更は、窓に紐づく行(例外設定・Rolling のサイクル)を指す先をずらしてしまう
        const Goal old = GetGoalById(goal.goal_id);
        const bool window_changed = old.goal_id != -1 &&
            (old.period_type != goal.period_type || old.period_n != goal.period_n || old.start_time != goal.start_time);
        if (window_changed) {
            if (HasGoalWindowOverride(goal.goal_id)) {
                std::cerr << "UpdateGoal Error: goal has target overrides; period_type / period_n / start_time are locked" << std::endl;
                return false;
            }
            // 例外設定がなければ、残っているのは Rolling のサイクル履歴だけ。起点が変わるので作り直す
            sql << "DELETE FROM goal_window WHERE goal_id = :goal_id", soci::use(goal.goal_id, "goal_id");
        }

        soci::indicator category_ind = (goal.category_id == -1) ? soci::i_null : soci::i_ok;
        soci::indicator todo_ind     = (goal.todo_id == -1)     ? soci::i_null : soci::i_ok;
        soci::indicator period_n_ind = (goal.period_n == -1)    ? soci::i_null : soci::i_ok;
        soci::indicator end_ind      = goal.end_time.empty()    ? soci::i_null : soci::i_ok;

        sql << "UPDATE goal SET category_id = :category_id, todo_id = :todo_id, goal_name = :goal_name, "
               "period_type = :period_type, period_n = :period_n, target_time = :target_time, "
               "start_time = :start_time, end_time = :end_time, is_active = :is_active, memo = :memo "
               "WHERE id = :goal_id",
            soci::use(goal.category_id, category_ind, "category_id"),
            soci::use(goal.todo_id, todo_ind, "todo_id"),
            soci::use(goal.goal_name, "goal_name"),
            soci::use(goal.period_type, "period_type"),
            soci::use(goal.period_n, period_n_ind, "period_n"),
            soci::use(goal.target_time, "target_time"),
            soci::use(goal.start_time, "start_time"),
            soci::use(goal.end_time, end_ind, "end_time"),
            soci::use(goal.is_active, "is_active"),
            soci::use(goal.memo, "memo"),
            soci::use(goal.goal_id, "goal_id");
    } catch (const soci::soci_error& e) {
        std::cerr << "UpdateGoal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

std::vector<Database::Goal> Database::GetGoalsByTarget(int category_id, int todo_id) {
    std::vector<Goal> results;
    if (sql.get_backend() == nullptr) return results;
    if (category_id == -1 && todo_id == -1) return results;

    try {
        // 対象の ID だけを取得し、中身は GetGoalById に任せる
        std::vector<int> ids;
        if (category_id != -1) {
            soci::rowset<int> rs = (sql.prepare << "SELECT id FROM goal WHERE category_id = :category_id ORDER BY id",
                                    soci::use(category_id, "category_id"));
            for (int id : rs) ids.push_back(id);
        } else {
            soci::rowset<int> rs = (sql.prepare << "SELECT id FROM goal WHERE todo_id = :todo_id ORDER BY id",
                                    soci::use(todo_id, "todo_id"));
            for (int id : rs) ids.push_back(id);
        }

        for (int id : ids) {
            Goal g = GetGoalById(id);
            if (g.goal_id != -1) results.push_back(g);
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "GetGoalsByTarget Error: " << e.what() << std::endl;
    }
    return results;
}

std::vector<Database::Goal> Database::GetAllGoals() {
    std::vector<Goal> results;
    if (sql.get_backend() == nullptr) return results;

    try {
        std::vector<int> ids;
        soci::rowset<int> rs = (sql.prepare << "SELECT id FROM goal ORDER BY id");
        for (int id : rs) ids.push_back(id);

        for (int id : ids) {
            Goal g = GetGoalById(id);
            if (g.goal_id != -1) results.push_back(g);
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "GetAllGoals Error: " << e.what() << std::endl;
    }
    return results;
}

bool Database::DeleteGoal(int goal_id) {
    if (sql.get_backend() == nullptr) return false;

    try {
        sql << "DELETE FROM goal WHERE id = :id", soci::use(goal_id, "id");
    } catch (const soci::soci_error& e) {
        std::cerr << "DeleteGoal Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

std::vector<Database::GoalWindow> Database::GetGoalWindows(int goal_id) {
    std::vector<GoalWindow> results;
    if (sql.get_backend() == nullptr) return results;

    try {
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT id, goal_id, window_key, target_override, achieved_at, memo "
            "FROM goal_window WHERE goal_id = :goal_id ORDER BY window_key",
            soci::use(goal_id, "goal_id"));

        for (const soci::row& row : rs) {
            GoalWindow w;
            w.id              = (int)row.get<long long>(0);
            w.goal_id         = (int)row.get<long long>(1);
            w.window_key      = row.get<std::string>(2);
            w.target_override = row.get_indicator(3) == soci::i_null ? -1 : row.get<long long>(3);
            w.achieved_at     = row.get_indicator(4) == soci::i_null ? "" : row.get<std::string>(4);
            w.memo            = row.get_indicator(5) == soci::i_null ? "" : row.get<std::string>(5);
            results.push_back(w);
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "GetGoalWindows Error: " << e.what() << std::endl;
    }
    return results;
}

bool Database::SetGoalWindowAchieved(int goal_id, const std::string& window_key, const std::string& achieved_at) {
    if (sql.get_backend() == nullptr) return false;

    try {
        sql << "INSERT INTO goal_window (goal_id, window_key, achieved_at) VALUES (:goal_id, :window_key, :achieved_at) "
               "ON CONFLICT (goal_id, window_key) DO UPDATE SET achieved_at = excluded.achieved_at",
            soci::use(goal_id, "goal_id"),
            soci::use(window_key, "window_key"),
            soci::use(achieved_at, "achieved_at");
    } catch (const soci::soci_error& e) {
        std::cerr << "SetGoalWindowAchieved Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::SetGoalWindowOverride(int goal_id, const std::string& window_key, long long target_override, const std::string& memo) {
    if (sql.get_backend() == nullptr) return false;

    try {
        sql << "INSERT INTO goal_window (goal_id, window_key, target_override, memo) "
               "VALUES (:goal_id, :window_key, :target_override, :memo) "
               "ON CONFLICT (goal_id, window_key) DO UPDATE SET "
               "target_override = excluded.target_override, memo = excluded.memo",
            soci::use(goal_id, "goal_id"),
            soci::use(window_key, "window_key"),
            soci::use(target_override, "target_override"),
            soci::use(memo, "memo");
    } catch (const soci::soci_error& e) {
        std::cerr << "SetGoalWindowOverride Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::ClearGoalWindowOverride(int goal_id, const std::string& window_key) {
    if (sql.get_backend() == nullptr) return false;

    try {
        soci::transaction tr(sql);
        sql << "UPDATE goal_window SET target_override = NULL, memo = NULL "
               "WHERE goal_id = :goal_id AND window_key = :window_key",
            soci::use(goal_id, "goal_id"), soci::use(window_key, "window_key");
        sql << "DELETE FROM goal_window WHERE goal_id = :goal_id AND window_key = :window_key "
               "AND target_override IS NULL AND achieved_at IS NULL",
            soci::use(goal_id, "goal_id"), soci::use(window_key, "window_key");
        tr.commit();
    } catch (const soci::soci_error& e) {
        std::cerr << "ClearGoalWindowOverride Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::HasGoalWindowOverride(int goal_id) {
    if (sql.get_backend() == nullptr) return false;

    try {
        int count = 0;
        sql << "SELECT COUNT(*) FROM goal_window WHERE goal_id = :goal_id AND target_override IS NOT NULL",
            soci::into(count), soci::use(goal_id, "goal_id");
        return count > 0;
    } catch (const soci::soci_error& e) {
        std::cerr << "HasGoalWindowOverride Error: " << e.what() << std::endl;
        return false;
    }
}

Database::Goal Database::GetGoalById(int goal_id) {
    Goal g{};
    g.goal_id = -1; // 見つからなかった場合の目印
    if (sql.get_backend() == nullptr) return g;

    try {
        soci::rowset<soci::row> rs = (sql.prepare <<
            "SELECT id, category_id, todo_id, goal_name, period_type, period_n, target_time, start_time, end_time, is_active, memo "
            "FROM goal WHERE id = :id",
            soci::use(goal_id, "id"));

        auto it = rs.begin();
        if (it == rs.end()) return g;

        const soci::row& row = *it;
        g.goal_id     = (int)row.get<long long>(0);
        g.category_id = row.get_indicator(1) == soci::i_null ? -1 : (int)row.get<long long>(1);
        g.todo_id     = row.get_indicator(2) == soci::i_null ? -1 : (int)row.get<long long>(2);
        g.goal_name   = row.get_indicator(3) == soci::i_null ? "" : row.get<std::string>(3);
        g.period_type = (int)row.get<long long>(4);
        g.period_n    = row.get_indicator(5) == soci::i_null ? -1 : (int)row.get<long long>(5);
        g.target_time = row.get<long long>(6);
        g.start_time  = row.get<std::string>(7);
        g.end_time    = row.get_indicator(8) == soci::i_null ? "" : row.get<std::string>(8);
        g.is_active   = (int)row.get<long long>(9);
        g.memo        = row.get_indicator(10) == soci::i_null ? "" : row.get<std::string>(10);
    } catch (const soci::soci_error& e) {
        std::cerr << "GetGoalById Error: " << e.what() << std::endl;
        g.goal_id = -1;
    }
    return g;
}
