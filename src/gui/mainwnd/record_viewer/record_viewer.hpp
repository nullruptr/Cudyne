#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/datectrl.h>
#include <wx/timectrl.h>
#include <wx/dateevt.h>
#include "core/db/database.hpp"

class RecordViewer : public wxPanel {
public:
    RecordViewer(wxWindow* parent, Database& dbRef);
    void SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end);

private:
    void LoadRecords();
    void OnListSelect(wxListEvent& event);

    Database& m_db;
    wxDateTime m_current_start;
    wxDateTime m_current_end;

    wxListCtrl*       m_list;

    long m_selected_item = -1; // リスト上の行インデックス
    long m_selected_id   = -1; // records.id
};
