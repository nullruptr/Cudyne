#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"

class EditRecordDlg : public wxDialog {
public:
    static EditRecordDlg* ForNew(wxWindow* parent, Database& db, int category_id);
    static EditRecordDlg* ForUpdate(wxWindow* parent, Database& db, int record_id);

private:
    EditRecordDlg(wxWindow* parent, Database& db, int m_category_id, int m_record_id);
    Database& m_db;
    int m_category_id;
    int m_record_id;

    wxStaticText* m_st_category_name_ref;
    wxStaticText* m_st_category_id_ref;
    wxStaticText* m_st_category_path_ref;
    wxStaticText* m_st_record_id_ref;
    wxRadioBox* m_radio_box;
};
