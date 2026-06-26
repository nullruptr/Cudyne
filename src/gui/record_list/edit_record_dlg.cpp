#include "edit_record_dlg.hpp"
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
    if (m_record_id != -1) {
	memo->SetValue(wxString::FromUTF8(m_db.GetMemoByRecordId(m_record_id)));
	// 合わせて、状態をUpdateにする
	m_radio_box->SetSelection(1);
    }
    sizer->Add(memo, 1, wxALL | wxEXPAND, FromDIP(5));

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

    btn_cancel->Bind(wxEVT_BUTTON, &EditRecordDlg::OnCancel, this); 
}

void EditRecordDlg::OnCancel(wxCommandEvent& WXUNUSED(event)){
    Close(true);
}
