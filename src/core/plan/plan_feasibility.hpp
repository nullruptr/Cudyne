#pragma once
// 設定した予定で、有効な Goal を達成できるかを、Goal の窓ごとに判定する。仕様は plan.md
// 窓の列挙と実績の集計は goal_report.hpp、判定そのものは plan_check.hpp
#include <algorithm>
#include <climits>
#include <functional>
#include <string>
#include <vector>
#include "core/db/database.hpp"
#include "core/goal/goal_report.hpp"
#include "core/plan/plan_check.hpp"

namespace PlanFeasibility {

// 判定に使う予定。DB に保存済みのものと、表で編集中(未保存)のものを合わせたもの
struct PlanSpan {
    int category_id = -1;
    long long start = 0;
    long long end = 0;
};

// [from, to) と重なる予定を、すべてのカテゴリぶん返す関数。呼び出し側が、DB の予定に未保存の変更を重ねて返す。
// Goal の窓ごとの範囲 (PlanCheck::PlanRange) で呼ばれるので、表に出ている期間や行には左右されない
using PlansProvider = std::function<std::vector<PlanSpan>(long long from, long long to)>;

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
// category_filter が -1 以外のときは、そのカテゴリ以下に設定された Goal だけを判定する。
//
// from / to は「どの窓を判定するか」を選ぶ範囲で、判定に使う予定の範囲ではない。
// 予定は、判定する窓そのもの (窓のうち今以降の部分) の範囲で、plans_in_range から取り出す。
// そのため、Filter by period で表に出ていない予定も、窓の中にあれば数える。
inline std::vector<Item> Check(Database& db, const PlansProvider& plans_in_range,
                               long long from, long long to, long long now, int category_filter = -1) {
    std::vector<Item> items;

    // 判定する窓と、その Goal の対象 (Category 以下) のカテゴリ
    struct Judged {
        size_t item_index;
        std::vector<int> category_ids;
    };
    std::vector<Judged> to_judge;

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

        // この Goal の対象 (Category 以下) のカテゴリ
        std::vector<int> category_ids{g.category_id};
        std::vector<int> children;
        if (db.GetChildCategories(children, g.category_id)) category_ids.insert(category_ids.end(), children.begin(), children.end());

        for (const GoalReport::Row& row : GoalReport::List(db, g, from, to, now)) {
            Item item = base;
            item.window = row;
            items.push_back(item);
            to_judge.push_back({items.size() - 1, category_ids});
        }
    }
    if (to_judge.empty() || !plans_in_range) return items;

    // 判定するすべての窓がカバーされる範囲で、予定を 1 回だけ取り出す (窓ごとに DB へ問い合わせないため)
    auto deadline_of = [](const Item& item) {
        return item.window.end == GoalEval::kNone ? PlanCheck::kNoDeadline : item.window.end;
    };
    long long fetch_from = LLONG_MAX, fetch_to = LLONG_MIN;
    for (const Judged& j : to_judge) {
        const Item& item = items[j.item_index];
        long long f = 0, t = 0;
        PlanCheck::PlanRange(item.window.start, deadline_of(item), now, f, t);
        fetch_from = std::min(fetch_from, f);
        fetch_to = std::max(fetch_to, t);
    }
    const std::vector<PlanSpan> plans = plans_in_range(fetch_from, fetch_to);

    // 窓ごとに、その Goal の対象の予定だけを渡して判定する (窓の外の分は、Judge が数えない)
    for (const Judged& j : to_judge) {
        Item& item = items[j.item_index];
        std::vector<PlanCheck::Span> scoped;
        for (const PlanSpan& p : plans) {
            if (std::find(j.category_ids.begin(), j.category_ids.end(), p.category_id) != j.category_ids.end()) {
                scoped.push_back({p.start, p.end});
            }
        }
        item.result = PlanCheck::Judge(item.window.target, item.window.achieved, now, item.window.start, deadline_of(item), scoped);
    }
    return items;
}

} // namespace PlanFeasibility
