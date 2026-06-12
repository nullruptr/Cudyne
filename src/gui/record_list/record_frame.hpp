#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/record_list/record_list_panel.hpp"

class RecordFrame : public wxFrame {
public:
    RecordFrame(wxWindow* parent, Database& dbRef,
                int category_id,
                const wxDateTime& start, const wxDateTime& end,
                const wxString& category_name);
private:
    RecordListPanel* m_panel;
};
