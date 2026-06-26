#include "edit_record_dlg.hpp"
#include "core/utils/format_time.hpp"
#include "core/db/database.hpp"
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/string.h>

// ファクトリメソッドパターン
// New or Update で場合分け
EditRecordDlg* EditRecordDlg::ForNew(wxWindow* parent, Database& db, int category_id) {
    return new EditRecordDlg(parent, db, category_id, -1);
}

EditRecordDlg* EditRecordDlg::ForUpdate(wxWindow* parent, Database& db, int record_id) {
    // 更新の時、カテゴリ情報を取得する必要があるため
    int category_id = db.GetCategoryIdByRecordId(record_id);
    return new EditRecordDlg(parent, db, category_id, record_id);
}

EditRecordDlg::EditRecordDlg(wxWindow* parent, Database &dbRef, int category_id, int record_id) 
    : wxDialog(parent, wxID_ANY, _("Edit Record"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) 
    , m_category_id(category_id)
    , m_record_id(record_id) {
    
    // Category Name: Name
    // Category Path: 
    // Category ID: 1
    // Record ID: 1
    //
    // ラジオボタン
    // Mode
    // New, Update
    //
    // Flex Grid Sizer
    // static text, DatePicker, wxTextCtrl(hh:mm), wxTextCtrl(ss)
    // Start Time: 2026 01/01 13:00 00
    // End Time  : 2026 01/01 15:00 00
    //
    // 日付、年は、DatePicker, 時間は、1300と入力したら、13:00などに変換する。
    // 秒も同じ
    //
    // メモ書き用のTextCtrl
    // Expand する。
    
    SetSize(FromDIP(wxSize(400, 500)));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // info
    wxFlexGridSizer* info_sizer = new wxFlexGridSizer(4, 2, FromDIP(5), FromDIP(5));

    wxStaticText* st_category_name = new wxStaticText(this, wxID_ANY, _("Category Name: "));
    m_st_category_name_ref = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_db.GetCategoryName(m_category_id)));
    wxStaticText* st_category_path = new wxStaticText(this, wxID_ANY, _("Category Path: "));
    m_st_category_path_ref = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id)));
    wxStaticText* st_category_id = new wxStaticText(this, wxID_ANY, _("Category ID: "));
    m_st_category_id_ref = new wxStaticText(this, wxID_ANY, wxString::Format("%d", m_category_id));
    wxStaticText* st_record_id = new wxStaticText(this, wxID_ANY, _("Record ID: "));
    m_st_record_id_ref = new wxStaticText(this, wxID_ANY, wxString::Format("%d", m_record_id));

    info_sizer->Add(st_category_name, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_name_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_category_path, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_path_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_category_id, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_category_id_ref, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(st_record_id, 0, wxALIGN_CENTER_VERTICAL);
    info_sizer->Add(m_st_record_id_ref, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(info_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    // 状態
    wxArrayString choices;
    choices.Add(_("New"));
    choices.Add(_("Update"));
    m_radio_box = new wxRadioBox(this, wxID_ANY, _("Mode"),
    wxDefaultPosition, wxDefaultSize, choices, 1, wxRA_SPECIFY_COLS);

    sizer->Add(m_radio_box, 0, wxALL | wxEXPAND, FromDIP(5));

    // 日付選択
    wxFlexGridSizer* time_sizer = new wxFlexGridSizer(2, 4, FromDIP(5), FromDIP(5));

    m_dp_start      = new wxDatePickerCtrl(this, wxID_ANY);
    m_tc_start_hhmm = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_start_ss   = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);

    m_dp_end        = new wxDatePickerCtrl(this, wxID_ANY);
    m_tc_end_hhmm   = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_end_ss     = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);

    time_sizer->Add(new wxStaticText(this, wxID_ANY, _("Start Time: ")), 0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_dp_start,      0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_tc_start_hhmm, 0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_tc_start_ss,   0, wxALIGN_CENTER_VERTICAL);

    time_sizer->Add(new wxStaticText(this, wxID_ANY, _("End Time: ")),   0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_dp_end,        0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_tc_end_hhmm,   0, wxALIGN_CENTER_VERTICAL);
    time_sizer->Add(m_tc_end_ss,     0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(time_sizer, 0, wxALL | wxEXPAND, FromDIP(5));

    // メモ
    wxTextCtrl* memo = new wxTextCtrl(this, wxID_ANY, "",
    wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    sizer->Add(memo, 1, wxALL | wxEXPAND, FromDIP(5));

    // Update モードの時
    if (m_record_id != -1) {
	memo->SetValue(wxString::FromUTF8(m_db.GetMemoByRecordId(m_record_id)));
	m_radio_box->SetSelection(1); // Update ラジオボタンを選択

	Database::Record rec = m_db.GetTimeByRecordId(m_record_id);

	TimeUtils::ParsedTime begin = TimeUtils::ParseEpoch(rec.time_begin);
	m_dp_start->SetValue(begin.date);
	m_tc_start_hhmm->SetValue(begin.hhmm);
	m_tc_start_ss->SetValue(begin.ss);

	TimeUtils::ParsedTime end = TimeUtils::ParseEpoch(rec.time_end);
	m_dp_end->SetValue(end.date);
	m_tc_end_hhmm->SetValue(end.hhmm);
	m_tc_end_ss->SetValue(end.ss);
    }

    // ボタン (下部)
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL); // 下部ボタン用サイザ
    
    wxButton* btn_save = new wxButton(
		    this,
		    wxID_ANY,
		    _("Save")
		    );

    wxButton* btn_cancel = new wxButton(
		    this,
		    wxID_EXIT,
		    _("Cancel")
		    );

    bottom_sizer->AddStretchSpacer(1); // 下部に余白を追加
    bottom_sizer->Add(btn_save, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(btn_cancel, 0, wxALL, FromDIP(5));

    sizer->Add(bottom_sizer, 0, wxEXPAND);

    
    SetSizer(sizer);
    CenterOnParent(); // 親ウィンドウの真ん中に表示する

    btn_save->Bind(wxEVT_BUTTON, &EditRecordDlg::OnSave, this);
    btn_cancel->Bind(wxEVT_BUTTON, &EditRecordDlg::OnCancel, this);

    // 時刻入力変換
    auto on_hhmm_enter = [this](wxCommandEvent& e) {
        OnValidateHHMM(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
    };
    auto on_hhmm_focus = [this](wxFocusEvent& e) {
        OnValidateHHMM(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
        e.Skip();
    };
    m_tc_start_hhmm->Bind(wxEVT_TEXT_ENTER, on_hhmm_enter);
    m_tc_end_hhmm->Bind(wxEVT_TEXT_ENTER, on_hhmm_enter);
    m_tc_start_hhmm->Bind(wxEVT_KILL_FOCUS, on_hhmm_focus);
    m_tc_end_hhmm->Bind(wxEVT_KILL_FOCUS, on_hhmm_focus);

    // 秒入力変換
    auto on_ss_enter = [this](wxCommandEvent& e) {
        OnValidateSS(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
    };
    auto on_ss_focus = [this](wxFocusEvent& e) {
        OnValidateSS(dynamic_cast<wxTextCtrl*>(e.GetEventObject()));
        e.Skip();
    };
    m_tc_start_ss->Bind(wxEVT_TEXT_ENTER, on_ss_enter);
    m_tc_end_ss->Bind(wxEVT_TEXT_ENTER, on_ss_enter);
    m_tc_start_ss->Bind(wxEVT_KILL_FOCUS, on_ss_focus);
    m_tc_end_ss->Bind(wxEVT_KILL_FOCUS, on_ss_focus);
}

void EditRecordDlg::OnSave(wxCommandEvent& event) {
    if (m_tc_start_hhmm->GetValue().IsEmpty() ||
        m_tc_start_ss->GetValue().IsEmpty()   ||
        m_tc_end_hhmm->GetValue().IsEmpty()   ||
        m_tc_end_ss->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter valid time"), "Error", wxOK | wxICON_WARNING);
        return;
    }
    // TODO:保存処理へ
}

void EditRecordDlg::OnCancel(wxCommandEvent& WXUNUSED(event)){
    Close(true);
}

void EditRecordDlg::OnValidateHHMM(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseHHMM(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid time"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
}

void EditRecordDlg::OnValidateSS(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseSS(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
}
