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
