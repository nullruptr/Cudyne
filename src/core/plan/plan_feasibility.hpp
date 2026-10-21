#pragma once
// 設定した予定で、有効な Goal を達成できるかを、Goal の窓ごとに判定する。仕様は plan.md
// 窓の列挙と実績の集計は goal_report.hpp、判定そのものは plan_check.hpp
#include <algorithm>
#include <string>
#include <vector>
#include "core/db/database.hpp"
#include "core/goal/goal_report.hpp"
#include "core/plan/plan_check.hpp"

namespace PlanFeasibility {

// 判定に使う予定。DB に保存済みのものと、表で編集中(未保存)のものを合わせて渡す
struct PlanSpan {
    int category_id = -1;
    long long start = 0;
    long long end = 0;
};

struct Item {
    int goal_id = -1;
    std::string goal_name;              // 名前がなければ "#ID"
    std::string subject;                // 対象のカテゴリのパス
    bool applicable = true;             // false: 判定できない (理由は reason)
    std::string reason;
    GoalReport::Row window;             // 判定した窓 (applicable = true のとき)
    PlanCheck::Result result;
};

// category_id 自身と、その配下 (フォルダも含む) すべての Category ID
inline std::vector<int> CategoryAndDescendants(Database& db, int category_id) {
    std::vector<int> ids{category_id};
    std::vector<Database::Category> all;
    if (!db.GetAllCategories(all)) return ids;

    // 親から子へ、順にたどる (ids に足しながら進む)
    for (size_t i = 0; i < ids.size(); ++i) {
        for (const Database::Category& c : all) {
            if (c.parent_id == ids[i] && std::find(ids.begin(), ids.end(), c.id) == ids.end()) ids.push_back(c.id);
        }
    }
    return ids;
}

// [from, to) と重なる窓を、有効な Goal ごとに判定する。窓は、終わっていないものだけを渡すこと (from >= now)。
// ToDo に設定した Goal は、予定が ToDo と結びついていないため判定できない。
// category_filter が -1 以外のときは、そのカテゴリ以下に設定された Goal だけを判定する
// (判定に使う予定は、絞り込みに関係なく、Goal の対象 (Category 以下) の全予定)
inline std::vector<Item> Check(Database& db, const std::vector<PlanSpan>& plans,
                               long long from, long long to, long long now, int category_filter = -1) {
    std::vector<Item> items;

    std::vector<int> shown_categories;
    if (category_filter != -1) shown_categories = CategoryAndDescendants(db, category_filter);

    for (const Database::Goal& g : db.GetAllGoals()) {
        if (g.is_active != 1) continue;

        // 絞り込み中は、対象のカテゴリ以下の Goal だけ (ToDo に設定した Goal は、カテゴリを持たないので除く)
        if (category_filter != -1 &&
            (g.category_id == -1 ||
             std::find(shown_categories.begin(), shown_categories.end(), g.category_id) == shown_categories.end())) {
            continue;
        }

        Item base;
        base.goal_id = g.goal_id;
        base.goal_name = g.goal_name.empty() ? "#" + std::to_string(g.goal_id) : g.goal_name;

        if (g.category_id == -1) {
            base.applicable = false;
            base.reason = "ToDo goal (plans are not linked to a ToDo)";
            items.push_back(base);
            continue;
        }
        base.subject = db.GetCategoriesPath(g.category_id);

        // この Goal の対象 (Category 以下) の予定だけを渡す
        std::vector<int> category_ids{g.category_id};
        std::vector<int> children;
        if (db.GetChildCategories(children, g.category_id)) category_ids.insert(category_ids.end(), children.begin(), children.end());

        std::vector<PlanCheck::Span> scoped;
        for (const PlanSpan& p : plans) {
            if (std::find(category_ids.begin(), category_ids.end(), p.category_id) != category_ids.end()) {
                scoped.push_back({p.start, p.end});
            }
        }

        for (const GoalReport::Row& row : GoalReport::List(db, g, from, to, now)) {
            Item item = base;
            item.window = row;
            item.result = PlanCheck::Judge(row.target, row.achieved, now, row.start,
                                           row.end == GoalEval::kNone ? PlanCheck::kNoDeadline : row.end, scoped);
            items.push_back(item);
        }
    }
    return items;
}

} // namespace PlanFeasibility
