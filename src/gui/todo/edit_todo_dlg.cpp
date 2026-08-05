#include "edit_todo_dlg.hpp"
#include "core/db/database.hpp"
#include "core/utils/format_time.hpp"
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/wx.h>

// ファクトリメソッドパターン
// New or Update で場合分け
EditTodoDlg* EditTodoDlg::ForNew(wxWindow* parent, Database& db) {
    return new EditTodoDlg(parent, db, -1, -1); // カテゴリ未選択の状態で開く
}

EditTodoDlg* EditTodoDlg::ForNew(wxWindow* parent, Database& db, int category_id) {
    return new EditTodoDlg(parent, db, category_id, -1); // カテゴリ指定済みの状態で開く
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

    SetSize(FromDIP(wxSize(450, 550)));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // info
    wxBoxSizer* sel_category_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxFlexGridSizer* info_sizer = new wxFlexGridSizer(4, 2, FromDIP(5), FromDIP(5));

    wxButton* btn_sel_category = new wxButton(this, wxID_ANY, _("Select Category"));
    wxButton* btn_clear_category = new wxButton(this, wxID_ANY, _("Clear"));
    wxStaticText* st_category_name = new wxStaticText(this, wxID_ANY, _("Category Name: "));
    m_st_category_name_ref = new wxStaticText(this, wxID_ANY, "-");
    wxStaticText* st_category_path = new wxStaticText(this, wxID_ANY, _("Category Path: "));
    m_st_category_path_ref = new wxStaticText(this, wxID_ANY, "-");
    wxStaticText* st_category_id = new wxStaticText(this, wxID_ANY, _("Category ID: "));
    m_st_category_id_ref = new wxStaticText(this, wxID_ANY, "-");
    wxStaticText* st_todo_id = new wxStaticText(this, wxID_ANY, _("Todo ID: "));
    m_st_todo_id_ref = new wxStaticText(this, wxID_ANY, "-");

    sel_category_sizer->Add(btn_sel_category, 0, wxALIGN_CENTER_VERTICAL);
    sel_category_sizer->Add(btn_clear_category, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
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
    btn_clear_category->Bind(wxEVT_BUTTON, &EditTodoDlg::OnClearCategory, this);
    UpdateCategoryLabels();

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

    // 日付・時刻選択 (Start Time, Target End, Deadline)
    wxFlexGridSizer* date_sizer = new wxFlexGridSizer(3, 5, FromDIP(5), FromDIP(5));

    // wxDP_DROPDOWN でカレンダーのドロップダウンから日付を選べるようにする
    const long date_picker_style = wxDP_DROPDOWN | wxDP_SHOWCENTURY;

    m_dp_start          = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_start_hhmm     = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_start_ss       = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);

    m_dp_target_end     = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_target_end_hhmm = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_target_end_ss  = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);

    m_dp_deadline       = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_deadline_hhmm  = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_deadline_ss    = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);

    wxButton* btn_now_start = new wxButton(this, wxID_ANY, _("Now"));

    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Start Time: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_start,      0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_start_hhmm, 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_start_ss,   0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(btn_now_start,   0, wxALIGN_CENTER_VERTICAL);

    wxButton* btn_copy_deadline_to_target_end = new wxButton(this, wxID_ANY, _("= Deadline"));

    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Target End: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_target_end,     0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_target_end_hhmm, 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_target_end_ss,  0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(btn_copy_deadline_to_target_end, 0, wxALIGN_CENTER_VERTICAL);

    date_sizer->Add(new wxStaticText(this, wxID_ANY, _("Deadline: ")), 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_dp_deadline,      0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_deadline_hhmm, 0, wxALIGN_CENTER_VERTICAL);
    date_sizer->Add(m_tc_deadline_ss,   0, wxALIGN_CENTER_VERTICAL);
    date_sizer->AddSpacer(0);

    btn_copy_deadline_to_target_end->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_dp_target_end->SetValue(m_dp_deadline->GetValue());
        m_tc_target_end_hhmm->SetValue(m_tc_deadline_hhmm->GetValue());
        m_tc_target_end_ss->SetValue(m_tc_deadline_ss->GetValue());
    });

    btn_now_start->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxDateTime now = wxDateTime::Now();
        m_dp_start->SetValue(now);
        m_tc_start_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_start_ss->SetValue(now.Format("%S"));
    });

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

        // DB には UTC で保存されているので、システム(ローカル)時間に変換して表示する
        TimeUtils::ParsedTime start = TimeUtils::ParseUTCString(todo.time_begin);
        m_dp_start->SetValue(start.date);
        m_tc_start_hhmm->SetValue(start.hhmm);
        m_tc_start_ss->SetValue(start.ss);

        TimeUtils::ParsedTime target_end = TimeUtils::ParseUTCString(todo.target_end);
        m_dp_target_end->SetValue(target_end.date);
        m_tc_target_end_hhmm->SetValue(target_end.hhmm);
        m_tc_target_end_ss->SetValue(target_end.ss);

        TimeUtils::ParsedTime deadline = TimeUtils::ParseUTCString(todo.deadline);
        m_dp_deadline->SetValue(deadline.date);
        m_tc_deadline_hhmm->SetValue(deadline.hhmm);
        m_tc_deadline_ss->SetValue(deadline.ss);

        m_tc_memo->SetValue(wxString::FromUTF8(todo.memo));

        m_radio_box->SetSelection(1); // Update ラジオボタンを選択
        m_st_todo_id_ref->SetLabel(wxString::Format("%d", m_todo_id));
    }

    // ForNew のとき Update ラジオボタンを無効化
    // (ToDo は Record と違い一覧側にカテゴリ文脈がないため、New でもカテゴリ選択は有効のまま)
    if (m_todo_id == -1) {
        m_radio_box->Enable(1, false); // index 1 = Update

        // Start Time のデフォルトを現在時刻にする
        wxDateTime now = wxDateTime::Now();
        m_dp_start->SetValue(now);
        m_tc_start_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_start_ss->SetValue(now.Format("%S"));
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

    // 時刻入力変換
    auto on_hhmm_enter = [this](wxCommandEvent& e) {
        OnValidateHHMM(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
    };
    auto on_hhmm_focus = [this](wxFocusEvent& e) {
        OnValidateHHMM(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
        e.Skip();
    };
    m_tc_start_hhmm->Bind(wxEVT_TEXT_ENTER, on_hhmm_enter);
    m_tc_target_end_hhmm->Bind(wxEVT_TEXT_ENTER, on_hhmm_enter);
    m_tc_deadline_hhmm->Bind(wxEVT_TEXT_ENTER, on_hhmm_enter);
    m_tc_start_hhmm->Bind(wxEVT_KILL_FOCUS, on_hhmm_focus);
    m_tc_target_end_hhmm->Bind(wxEVT_KILL_FOCUS, on_hhmm_focus);
    m_tc_deadline_hhmm->Bind(wxEVT_KILL_FOCUS, on_hhmm_focus);

    // 秒入力変換
    auto on_ss_enter = [this](wxCommandEvent& e) {
        OnValidateSS(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
    };
    auto on_ss_focus = [this](wxFocusEvent& e) {
        OnValidateSS(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
        e.Skip();
    };
    m_tc_start_ss->Bind(wxEVT_TEXT_ENTER, on_ss_enter);
    m_tc_target_end_ss->Bind(wxEVT_TEXT_ENTER, on_ss_enter);
    m_tc_deadline_ss->Bind(wxEVT_TEXT_ENTER, on_ss_enter);
    m_tc_start_ss->Bind(wxEVT_KILL_FOCUS, on_ss_focus);
    m_tc_target_end_ss->Bind(wxEVT_KILL_FOCUS, on_ss_focus);
    m_tc_deadline_ss->Bind(wxEVT_KILL_FOCUS, on_ss_focus);

    // Update モード、かつ元々 target_end / deadline が未設定だった場合のために
    // hh:mm, ss が空のままだと保存時にエラーになるので、デフォルト値を入れておく
    if (m_tc_start_hhmm->GetValue().IsEmpty())      { m_tc_start_hhmm->SetValue("00:00");      m_tc_start_ss->SetValue("00"); }
    if (m_tc_target_end_hhmm->GetValue().IsEmpty()) { m_tc_target_end_hhmm->SetValue("00:00"); m_tc_target_end_ss->SetValue("00"); }
    if (m_tc_deadline_hhmm->GetValue().IsEmpty())   { m_tc_deadline_hhmm->SetValue("00:00");   m_tc_deadline_ss->SetValue("00"); }
}

void EditTodoDlg::OnSave(wxCommandEvent& event) {
    if (m_tc_todo_name->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter a todo name"), "Error", wxOK | wxICON_WARNING);
        return;
    }

    if (m_tc_start_hhmm->GetValue().IsEmpty()      || m_tc_start_ss->GetValue().IsEmpty()      ||
        m_tc_target_end_hhmm->GetValue().IsEmpty() || m_tc_target_end_ss->GetValue().IsEmpty() ||
        m_tc_deadline_hhmm->GetValue().IsEmpty()   || m_tc_deadline_ss->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter valid time"), "Error", wxOK | wxICON_WARNING);
        return;
    }

    Database::ToDo todo{};
    todo.category_id = m_category_id;
    todo.priority    = m_sc_priority->GetValue();
    todo.todo_name   = m_tc_todo_name->GetValue().utf8_string();
    todo.memo        = m_tc_memo->GetValue().utf8_string();

    // 表示はシステム(ローカル)時間、保存は UTC
    todo.time_begin = TimeUtils::BuildUTCString(m_dp_start->GetValue(),      m_tc_start_hhmm->GetValue(),      m_tc_start_ss->GetValue());
    todo.target_end = TimeUtils::BuildUTCString(m_dp_target_end->GetValue(), m_tc_target_end_hhmm->GetValue(), m_tc_target_end_ss->GetValue());
    todo.deadline   = TimeUtils::BuildUTCString(m_dp_deadline->GetValue(),   m_tc_deadline_hhmm->GetValue(),   m_tc_deadline_ss->GetValue());

    if (m_cb_done->GetValue()) {
        todo.status = 1;
        // 完了に切り替わった瞬間だけ完了日を確定させる。既に完了日があれば据え置く
        todo.completion_date = m_completion_date.empty() ? wxDateTime::Now().ToUTC().Format("%Y-%m-%d %H:%M:%S").utf8_string() : m_completion_date;
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
        UpdateCategoryLabels();
    }
}

void EditTodoDlg::OnClearCategory(wxCommandEvent& WXUNUSED(event)) {
    m_category_id = -1;
    UpdateCategoryLabels();
}

void EditTodoDlg::UpdateCategoryLabels() {
    if (m_category_id == -1) {
        m_st_category_name_ref->SetLabel("-");
        m_st_category_path_ref->SetLabel("-");
        m_st_category_id_ref->SetLabel("-");
        return;
    }

    m_st_category_name_ref->SetLabel(wxString::FromUTF8(m_db.GetCategoryName(m_category_id)));
    m_st_category_path_ref->SetLabel(wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id)));
    m_st_category_id_ref->SetLabel(wxString::Format("%d", m_category_id));
}

void EditTodoDlg::OnValidateHHMM(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseHHMM(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid time"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
}

void EditTodoDlg::OnValidateSS(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseSS(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
}
