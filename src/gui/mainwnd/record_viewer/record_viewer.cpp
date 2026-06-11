#include "record_viewer.hpp"
#include <wx/statbox.h>

RecordViewer::RecordViewer(wxWindow* parent, Database& dbRef)
    : wxPanel(parent, wxID_ANY), m_db(dbRef)
{
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    // --- リスト ---
    m_list = new wxListCtrl(this, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->InsertColumn(0, _("ID"),       wxLIST_FORMAT_LEFT,  FromDIP(40));
    m_list->InsertColumn(1, _("Category"), wxLIST_FORMAT_LEFT,  FromDIP(120));
    m_list->InsertColumn(2, _("Begin"),    wxLIST_FORMAT_LEFT,  FromDIP(130));
    m_list->InsertColumn(3, _("End"),      wxLIST_FORMAT_LEFT,  FromDIP(130));
    m_list->InsertColumn(4, _("Total"),    wxLIST_FORMAT_LEFT,  FromDIP(130));
    root->Add(m_list, 1, wxEXPAND | wxALL, FromDIP(4));

    SetSizer(root);

    // イベント
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED, &RecordViewer::OnListSelect, this);
}

void RecordViewer::SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end) {
    m_current_start = start;
    m_current_end   = end;
    m_selected_item = -1;
    m_selected_id   = -1;
    LoadRecords();
}

void RecordViewer::LoadRecords() {
    m_list->DeleteAllItems();
    // TODO: DB から個別レコードを取得
}

void RecordViewer::OnListSelect(wxListEvent& event) {
    m_selected_item = event.GetIndex();
    // TODO: 選択行の処理
}
