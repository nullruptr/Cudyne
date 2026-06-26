#include "edit_record_dlg.hpp"

EditRecordDlg::EditRecordDlg(wxWindow* parent, Database &dbRef, int m_record_id) 
    : wxDialog(parent, wxID_ANY, _("Edit Record"), wxDefaultPosition, wxDefaultSize)
    , m_db(dbRef) {
    
    // Category Name: Name
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
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
}
