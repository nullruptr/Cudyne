#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/spinctrl.h>
#include "core/db/database.hpp"

class EditGoalDlg : public wxDialog {
public:
    EditGoalDlg(wxWindow* parent, Database& db);

private:
    Database& m_db;

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

    wxSpinCtrlDouble* m_sc_target_value;
    wxChoice* m_ch_target_unit;      // seconds / minutes / hour
    wxStaticText* m_st_target_sec;   // 秒換算の表示

    wxStaticText* m_st_constraint;   // 親カテゴリの目標による制約
    wxButton* m_btn_edit_parent_goal;

    wxTextCtrl* m_tc_memo;

    void OnSave(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnSelect(wxCommandEvent& event);
    void OnValidateHHMM(wxTextCtrl* tc);
    void OnValidateSS(wxTextCtrl* tc);
    void UpdateSelectedName();      // 選択中の Category / ToDo 名を更新
    void UpdatePeriodControls();    // Every n Days のときだけ n を有効化
    void UpdateTargetSeconds();     // 目標時間の秒換算を更新
    long long GetTargetSeconds() const;
};
