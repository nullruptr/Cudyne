#include "record_frame.hpp"

RecordFrame::RecordFrame(wxWindow* parent, Database& dbRef,
                         int category_id,
                         const wxDateTime& start, const wxDateTime& end,
                         const wxString& category_name)
    : wxFrame(parent, wxID_ANY,
              wxString::Format(_("Records - %s (%s ~ %s)"),
                               category_name,
                               start.FormatISODate(),
                               end.FormatISODate()),
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
{
    SetSize(FromDIP(wxSize(700, 500)));
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_panel = new RecordListPanel(this, dbRef);
    sizer->Add(m_panel, 1, wxEXPAND);
    SetSizer(sizer);
    m_panel->LoadRecords(start, end, category_id);
    CenterOnParent();
}
