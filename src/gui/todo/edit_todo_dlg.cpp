#include "edit_todo_dlg.hpp"
#include "core/db/database.hpp"
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace {
    // "YYYY-MM-DD" -> wxDateTime。空文字や不正値は今日の日付にする
    wxDateTime ParseIsoDate(const std::string& str) {
        wxDateTime dt;
        if (str.empty() || !dt.ParseISODate(wxString::FromUTF8(str))) {
            dt = wxDateTime::Today();
        }
        return dt;
    }
}

// ファクトリメソッドパターン
// New or Update で場合分け
EditTodoDlg* EditTodoDlg::ForNew(wxWindow* parent, Database& db, int category_id) {
    return new EditTodoDlg(parent, db, category_id, -1);
}

EditTodoDlg* EditTodoDlg::ForUpdate(wxWindow* parent, Database& db, int todo_id) {
    Database::ToDo todo = db.GetTodoById(todo_id);
    return new EditTodoDlg(parent, db, todo.category_id, todo_id);
}

EditTodoDlg::EditTodoDlg(wxWindow* parent, Database& dbRef, int category_id, int todo_id)
    : wxDialog(parent, wxID_ANY, _("Edit ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef)
    , m_category_id(category_id)
    , m_todo_id(todo_id) {

    SetSize(FromDIP(wxSize(400, 500)));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // info
    wxBoxSizer* sel_category_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer* info_sizer = new wxFlexGridSizer(4, 2, FromDIP(5), FromDIP(5));

    wxButton* btn_sel_category = new wxButton(this, wxID_ANY, _("Select Category"));
    wxStaticText* st_category_name = new wxStaticText(this, wxID_ANY, _("Category Name: "));
    m_st_category_name_ref = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_db.GetCategoryName(m_category_id)));
    wxStaticText* st_category_path = new wxStaticText(this, wxID_ANY, _("Category Path: "));
    m_st_category_path_ref = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id)));
    wxStaticText* st_category_id = new wxStaticText(this, wxID_ANY, _("Category ID: "));
    m_st_category_id_ref = new wxStaticText(this, wxID_ANY, wxString::Format("%d", m_category_id));
    wxStaticText* st_todo_id = new wxStaticText(this, wxID_ANY, _("Todo ID: "));
    m_st_todo_id_ref = new wxStaticText(this, wxID_ANY, "-");

    sel_category_sizer->Add(btn_sel_category, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_category_name, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_name_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_category_path, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_path_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_category_id, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_id_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_todo_id, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_todo_id_ref, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(sel_category_sizer, 0, wxALL | wxEXPAND, FromDIP(5));
    sizer->Add(info_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    btn_sel_category->Bind(wxEVT_BUTTON, &EditTodoDlg::OnSelCategory, this);

    // 状態
    wxArrayString choices;
    choices.Add(_("New"));
    choices.Add(_("Update"));
    m_radio_box = new wxRadioBox(this, wxID_ANY, _("Mode"),
        wxDefaultPosition, wxDefaultSize, choices, 1, wxRA_SPECIFY_COLS);

    sizer->Add(m_radio_box, 0, wxALL | wxEXPAND, FromDIP(5));

    // Todo Name
    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(this, wxID_ANY, _("Todo Name: ")), 0, wxALIGN_CENTER_VERTICAL);
    m_tc_todo_name = new wxTextCtrl(this, wxID_ANY, "");
    name_sizer->Add(m_tc_todo_name, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
    sizer->Add(name_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    // Priority, Done
    wxBoxSizer* status_sizer = new wxBoxSizer(wxHORIZONTAL);
    status_sizer->Add(new wxStaticText(this, wxID_ANY, _("Priority: ")), 0, wxALIGN_CENTER_VERTICAL);
    m_sc_priority = new wxSpinCtrl(this, wxID_ANY, "0", wxDefaultPosition, FromDIP(wxSize(60, -1)), wxSP_ARROW_KEYS, 0, 100, 0);
    status_sizer->Add(m_sc_priority, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
    m_cb_done = new wxCheckBox(this, wxID_ANY, _("Done"));
    status_sizer->Add(m_cb_done, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(20));
    sizer->Add(status_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    // 日付選択
    wxFlexGridSizer* date_sizer = new wxFlexGridSizer(3, 2, FromDIP(5), FromDIP(5));

    m_dp_start      = new wxDatePickerCtrl(this, wxID_ANY);
    m_dp_target_end = new wxDatePickerCtrl(this, wxID_ANY);
    m_dp_deadline   = new wxDatePickerCtrl(this, wxID_ANY);

    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Start Time: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_start, 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Target End: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_target_end, 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Deadline: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_deadline, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(date_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    // メモ
    m_tc_memo = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    sizer->Add(m_tc_memo, 1, wxALL | wxEXPAND, FromDIP(5));

    // Update モードの時
    if (m_todo_id != -1) {
        Database::ToDo todo = m_db.GetTodoById(m_todo_id);

        m_tc_todo_name->SetValue(wxString::FromUTF8(todo.todo_name));
        m_sc_priority->SetValue(todo.priority);
        m_cb_done->SetValue(todo.status == 1);
        m_completion_date = todo.completion_date;

        m_dp_start->SetValue(ParseIsoDate(todo.time_begin));
        m_dp_target_end->SetValue(ParseIsoDate(todo.target_end));
        m_dp_deadline->SetValue(ParseIsoDate(todo.deadline));

        m_tc_memo->SetValue(wxString::FromUTF8(todo.memo));

        m_radio_box->SetSelection(1); // Update ラジオボタンを選択
        m_st_todo_id_ref->SetLabel(wxString::Format("%d", m_todo_id));
    }

    // ForNew のとき Update, カテゴリ選択ボタンを無効化
    if (m_todo_id == -1) {
        m_radio_box->Enable(1, false); // index 1 = Update
        btn_sel_category->Enable(false);
    }

    // ラジオボタンの状態によって、Todo ID の表示を変える
    m_radio_box->Bind(wxEVT_RADIOBOX, [this](wxCommandEvent& e) {
        if (e.GetSelection() == 0) { // New
            m_st_todo_id_ref->SetLabel("-");
        } else { // Update
            m_st_todo_id_ref->SetLabel(wxString::Format("%d", m_todo_id));
        }
    });

    // ボタン (下部)
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* btn_save = new wxButton(this, wxID_ANY, _("Save"));
    wxButton* btn_cancel = new wxButton(this, wxID_EXIT, _("Cancel"));

    bottom_sizer->AddStretchSpacer(1);
    bottom_sizer->Add(btn_save, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(btn_cancel, 0, wxALL, FromDIP(5));

    sizer->Add(bottom_sizer, 0, wxEXPAND);

    SetSizer(sizer);
    CenterOnParent();

    btn_save->Bind(wxEVT_BUTTON, &EditTodoDlg::OnSave, this);
    btn_cancel->Bind(wxEVT_BUTTON, &EditTodoDlg::OnCancel, this);
}

void EditTodoDlg::OnSave(wxCommandEvent& event) {
    if (m_tc_todo_name->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter a todo name"), "Error", wxOK | wxICON_WARNING);
        return;
    }

    Database::ToDo todo{};
    todo.category_id     = m_category_id;
    todo.priority        = m_sc_priority->GetValue();
    todo.todo_name       = m_tc_todo_name->GetValue().utf8_string();
    todo.time_begin      = m_dp_start->GetValue().FormatISODate().utf8_string();
    todo.target_end      = m_dp_target_end->GetValue().FormatISODate().utf8_string();
    todo.deadline        = m_dp_deadline->GetValue().FormatISODate().utf8_string();
    todo.memo            = m_tc_memo->GetValue().utf8_string();

    if (m_cb_done->GetValue()) {
        todo.status = 1;
        // 完了に切り替わった瞬間だけ完了日を確定させる。既に完了日があれば据え置く
        todo.completion_date = m_completion_date.empty() ? wxDateTime::Today().FormatISODate().utf8_string() : m_completion_date;
    } else {
        todo.status = 0;
        todo.completion_date = "";
    }

    switch (m_radio_box->GetSelection()) {
    case 0: // New
        if (!m_db.InsertToDo(todo)) {
            wxMessageBox(_("Failed to save"), "Error", wxOK | wxICON_ERROR);
            return;
        }
        break;
    case 1: // Update
        todo.todo_id = m_todo_id;
        if (!m_db.UpdateToDo(todo)) {
            wxMessageBox(_("Failed to save"), "Error", wxOK | wxICON_ERROR);
            return;
        }
        break;
    default:
        break;
    }
    Close(true);
}

void EditTodoDlg::OnCancel(wxCommandEvent& WXUNUSED(event)) {
    Close(true);
}

void EditTodoDlg::OnSelCategory(wxCommandEvent& WXUNUSED(event)) {
    SelCategoryDlg dlg(this, m_db);
    if (dlg.ShowModal() == wxID_OK) {
        m_category_id = dlg.GetSelectedCategoryId();
        m_st_category_name_ref->SetLabel(wxString::FromUTF8(m_db.GetCategoryName(m_category_id)));
        m_st_category_path_ref->SetLabel(wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id)));
        m_st_category_id_ref->SetLabel(wxString::Format("%d", m_category_id));
    }
}
