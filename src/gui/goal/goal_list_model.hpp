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
    Col_Pace,    // 経過率と達成率
    Col_Start,
    Col_End,
    Col_Active,
    Col_Count // 列の数
};

struct GoalRow {
    int goal_id = -1;
    GoalEval::State state = GoalEval::State::Comfortable;
    wxString cells[Col_Count];

    // タイマーで DB に問い合わせず再計算するための、読み込み時点の値
    GoalEval::GoalSpec spec;
    long long achieved_base = 0;   // 読み込み時点の実績(秒)
    long long base_now = 0;        // 読み込み時点の時刻
    bool recording = false;        // 記録中なら、実績は時間とともに増える
    long long window_end = GoalEval::kNone;
};

class GoalListModel : public wxDataViewVirtualListModel {
public:
    GoalListModel() : wxDataViewVirtualListModel(0) {}

    // 行数が変わるときは Reset を使う
    void SetRows(std::vector<GoalRow> rows) {
        m_rows = std::move(rows);
        Reset(static_cast<unsigned>(m_rows.size()));
    }

    // 表示中の行と、goal_id の並びが同じか
    bool HasSameIds(const std::vector<GoalRow>& rows) const {
        if (rows.size() != m_rows.size()) return false;
        for (size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].goal_id != m_rows[i].goal_id) return false;
        }
        return true;
    }

    // 行の並びが同じときの更新。Reset しないので、選択やスクロールが保たれる
    void UpdateRows(std::vector<GoalRow> rows) {
        m_rows = std::move(rows);
        for (size_t i = 0; i < m_rows.size(); ++i) RowChanged(static_cast<unsigned>(i));
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
        case GoalEval::State::Behind:        attr.SetBackgroundColour(AppTheme::GetGoalBehind());        return true;
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
