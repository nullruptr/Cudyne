#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/spinctrl.h>
#include "core/db/database.hpp"

class EditGoalDlg : public wxDialog {
public:
    // goal_id != -1 のときは編集モード (保存済みの goal を読み込み、category_id / todo_id は無視する)
    EditGoalDlg(wxWindow* parent, Database& db, int category_id = -1, int todo_id = -1, int goal_id = -1);

private:
    Database& m_db;

    int m_goal_id = -1; // -1 = 新規

    int m_category_id = -1; // category_id / todo_id はどちらか一方のみ設定
    int m_todo_id = -1;

    wxTextCtrl* m_tc_name;
    wxChoice* m_ch_target_kind;      // Category / ToDo
    wxStaticText* m_st_selected_name;
    wxStaticText* m_st_selected_path;
    wxStaticText* m_st_selected_id;
    wxCheckBox* m_cb_active;

    wxChoice* m_ch_period_type;      // Daily / Weekly / Monthly / Every n Days
    wxSpinCtrl* m_sc_every_n;
    wxStaticText* m_st_days;

    wxDatePickerCtrl* m_dp_start;
    wxTextCtrl* m_tc_start_hhmm;
    wxTextCtrl* m_tc_start_ss;

    wxCheckBox* m_cb_end;            // ON のとき終了日時を設定する (OFF = 期限なし)
    wxDatePickerCtrl* m_dp_end;
    wxTextCtrl* m_tc_end_hhmm;
    wxTextCtrl* m_tc_end_ss;
    wxButton* m_btn_end_now;

    wxSpinCtrlDouble* m_sc_target_value;
    wxChoice* m_ch_target_unit;      // seconds / minutes / hour
    wxStaticText* m_st_target_warn;  // 目標時間が上限を超えているときの赤字の警告
    wxStaticText* m_st_target_max;  // "/ 最大稼働時間" の表示 (単位は m_ch_target_unit に従う)

    wxStaticText* m_st_constraint;   // 親カテゴリの目標による制約
    wxButton* m_btn_edit_parent_goal;

    wxTextCtrl* m_tc_memo;

    void OnSave(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSelect(wxCommandEvent& event);
    void OnValidateHHMM(wxTextCtrl* tc);
    void OnValidateSS(wxTextCtrl* tc);
    void UpdateSelectedName();      // 選択中の Category / ToDo 名を更新
    void UpdateEndControls();       // End Time のチェックに応じて入力欄を有効化
    void UpdatePeriodControls();   // Every n Days のときだけ n を有効化
    void UpdateTargetLimit();       // 最大稼働時間の表示を更新
    void UpdateTargetWarning();     // 上限超過の警告 (赤字) を更新
    long long GetTargetSeconds() const;
    long long GetPeriodSeconds() const; // 期間の長さ(秒) = 目標時間の数学的な上限
};
