#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/spinctrl.h>
#include "core/db/database.hpp"
#include "gui/common/sel_category_dlg/sel_category_dlg.hpp"

class EditTodoDlg : public wxDialog {
public:
    static EditTodoDlg* ForNew(wxWindow* parent, Database& db);
    static EditTodoDlg* ForNew(wxWindow* parent, Database& db, int category_id);
    static EditTodoDlg* ForUpdate(wxWindow* parent, Database& db, int todo_id);

private:
    EditTodoDlg(wxWindow* parent, Database& db, int category_id, int todo_id);
    Database& m_db;

    int m_category_id;
    int m_todo_id;

    wxStaticText* m_st_category_name_ref;
    wxStaticText* m_st_category_id_ref;
    wxStaticText* m_st_category_path_ref;
    wxStaticText* m_st_todo_id_ref;
    wxRadioBox* m_radio_box;

    wxTextCtrl* m_tc_todo_name;
    wxSpinCtrl* m_sc_priority;
    wxCheckBox* m_cb_done;

    wxDatePickerCtrl* m_dp_start;
    wxTextCtrl* m_tc_start_hhmm;
    wxTextCtrl* m_tc_start_ss;

    wxDatePickerCtrl* m_dp_target_end;
    wxTextCtrl* m_tc_target_end_hhmm;
    wxTextCtrl* m_tc_target_end_ss;

    wxDatePickerCtrl* m_dp_deadline;
    wxTextCtrl* m_tc_deadline_hhmm;
    wxTextCtrl* m_tc_deadline_ss;

    wxTextCtrl* m_tc_memo;

    std::string m_completion_date; // 完了日（完了時に確定させ、以後は保持する）

    void OnSave(wxCommandEvent& WXUNUSED(event)); // 保存
    void OnCancel(wxCommandEvent& WXUNUSED(event)); // キャンセル
    void OnSelCategory(wxCommandEvent& WXUNUSED(event)); // カテゴリ選択
    void OnClearCategory(wxCommandEvent& WXUNUSED(event)); // カテゴリ選択解除
    void OnValidateHHMM(wxTextCtrl* tc);
    void OnValidateSS(wxTextCtrl* tc);
    void UpdateCategoryLabels(); // m_category_id を元に表示ラベルを更新
};
