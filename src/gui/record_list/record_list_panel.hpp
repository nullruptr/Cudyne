#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "core/db/database.hpp"

class RecordListPanel : public wxPanel {
public:
    RecordListPanel(wxWindow* parent, Database& dbRef);
    void LoadRecords(const wxDateTime& start, const wxDateTime& end, int category_id = -1);

private:
    void OnListSelect(wxListEvent& event);
    void OnListDoubleClick(wxListEvent& event);

    Database&   m_db;
    wxListCtrl* m_list;
    long        m_selected_item = -1;
    long        m_selected_id   = -1;
};
