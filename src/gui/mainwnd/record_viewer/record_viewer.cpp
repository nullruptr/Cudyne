#include "record_viewer.hpp"

RecordViewer::RecordViewer(wxWindow* parent, Database& dbRef)
    : wxPanel(parent, wxID_ANY), m_db(dbRef)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_panel = new RecordListPanel(this, dbRef);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
}

void RecordViewer::SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end) {
    m_panel->LoadRecords(start, end);
}
