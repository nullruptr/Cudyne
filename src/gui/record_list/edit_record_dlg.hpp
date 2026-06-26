#include <wx/wx.h>
#include "core/db/database.hpp"

class EditRecordDlg : public wxDialog {
public:
    EditRecordDlg(wxWindow* parent, Database& db, int m_record_id);
private:
    Database& m_db;
    int m_record_id;
};
