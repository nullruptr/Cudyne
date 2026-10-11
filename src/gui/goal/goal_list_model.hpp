#pragma once
#include <wx/dataview.h>
#include <utility>
#include <vector>
#include "core/goal/goal_eval.hpp"
#include "gui/theme/apptheme.hpp"

// 一覧の列番号。AppendTextColumn の model_column と、GetValueByRow の col で共通に使う
enum GoalColumn {
    Col_Status,
    Col_Name,
    Col_Subject, // Category / ToDo
    Col_Period,
    Col_Target,
    Col_Achieved,
    Col_Remaining,
    Col_Slack,   // この時間以内に着手しないと間に合わない
    Col_Start,
    Col_End,
    Col_Active,
    Col_Count // 列の数
};

struct GoalRow {
    int goal_id = -1;
    GoalEval::State state = GoalEval::State::Comfortable;
    wxString cells[Col_Count];
};

class GoalListModel : public wxDataViewVirtualListModel {
public:
    GoalListModel() : wxDataViewVirtualListModel(0) {}

    // 行数が変わるときは Reset を使う
    void SetRows(std::vector<GoalRow> rows) {
        m_rows = std::move(rows);
        Reset(static_cast<unsigned>(m_rows.size()));
    }

    // row 行目の goal_id (範囲外は -1)
    int GetGoalId(unsigned row) const {
        return row < m_rows.size() ? m_rows[row].goal_id : -1;
    }

    unsigned GetColumnCount() const override { return Col_Count; }
    wxString GetColumnType(unsigned) const override { return "string"; }

    void GetValueByRow(wxVariant& value, unsigned row, unsigned col) const override {
        if (row < m_rows.size() && col < Col_Count) value = m_rows[row].cells[col];
    }
    bool SetValueByRow(const wxVariant&, unsigned, unsigned) override { return false; } // 編集不可

    // 状態の色。無効は行全体を灰色に、それ以外は Status のセルだけに付ける
    bool GetAttrByRow(unsigned row, unsigned col, wxDataViewItemAttr& attr) const override {
        if (row >= m_rows.size()) return false;
        const GoalEval::State state = m_rows[row].state;

        if (state == GoalEval::State::Disabled) {
            attr.SetBackgroundColour(AppTheme::GetGoalDisabledBg());
            attr.SetColour(AppTheme::GetGoalDisabledText());
            return true;
        }
        if (col != Col_Status) return false;

        switch (state) {
        case GoalEval::State::Achieved:      attr.SetBackgroundColour(AppTheme::GetGoalAchieved());      return true;
        case GoalEval::State::Urgent:        attr.SetBackgroundColour(AppTheme::GetGoalUrgent());        return true;
        case GoalEval::State::Missed:        attr.SetBackgroundColour(AppTheme::GetGoalMissed());        return true;
        case GoalEval::State::NotStarted:    attr.SetBackgroundColour(AppTheme::GetGoalNotStarted());    return true;
        case GoalEval::State::EndedAchieved: attr.SetBackgroundColour(AppTheme::GetGoalEndedAchieved()); return true;
        case GoalEval::State::EndedMissed:   attr.SetBackgroundColour(AppTheme::GetGoalEndedMissed());   return true;
        default: return false; // 余裕あり: 色なし
        }
    }

private:
    std::vector<GoalRow> m_rows;
};
