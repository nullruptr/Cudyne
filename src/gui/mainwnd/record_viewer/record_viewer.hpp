#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/record_list/record_list_panel.hpp"

class RecordViewer : public wxPanel {
public:
    RecordViewer(wxWindow* parent, Database& dbRef);
    void SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end);
private:
    Database&        m_db;
    RecordListPanel* m_panel;
};
