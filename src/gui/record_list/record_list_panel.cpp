#include "record_list_panel.hpp"
#include "core/utils/format_time.hpp"
#include "edit_record_dlg.hpp"

RecordListPanel::RecordListPanel(wxWindow* parent, Database& dbRef)
    : wxPanel(parent, wxID_ANY), m_db(dbRef)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_list = new wxListCtrl(this, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->InsertColumn(0, _("ID"),       wxLIST_FORMAT_LEFT, FromDIP(40));
    m_list->InsertColumn(1, _("Category"), wxLIST_FORMAT_LEFT, FromDIP(120));
    m_list->InsertColumn(2, _("Begin"),    wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(3, _("End"),      wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(4, _("Total"),    wxLIST_FORMAT_LEFT, FromDIP(80));
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);

    m_list->Bind(wxEVT_LIST_ITEM_SELECTED,  &RecordListPanel::OnListSelect,      this);
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RecordListPanel::OnListDoubleClick, this);
}

void RecordListPanel::LoadRecords(const wxDateTime& start, const wxDateTime& end, int category_id) {
    m_list->DeleteAllItems();
    m_selected_item = -1;
    m_selected_id   = -1;
    if (!start.IsValid()) return;

    std::string s_utc = start.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
    std::string e_utc = end.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();

    auto records = m_db.GetRecordList(s_utc, e_utc, category_id);
    for (const auto& r : records) {
        long idx = m_list->InsertItem(m_list->GetItemCount(),
                                      wxString::Format("%d", r.id));
        m_list->SetItem(idx, 1, wxString::FromUTF8(r.category_name));
        m_list->SetItem(idx, 2, TimeUtils::FormatEpochToDate(r.time_begin));
        m_list->SetItem(idx, 3, TimeUtils::FormatEpochToDate(r.time_end));
        m_list->SetItem(idx, 4, TimeUtils::FormatSeconds(r.total_seconds));
        m_list->SetItemData(idx, (long)r.id);
    }
}

void RecordListPanel::OnListSelect(wxListEvent& event) {
    m_selected_item = event.GetIndex();
    m_selected_id   = m_list->GetItemData(m_selected_item);
}

void RecordListPanel::OnListDoubleClick(wxListEvent& event) {
    long item = event.GetIndex();
    int record_id = (int)m_list->GetItemData(item);

    EditRecordDlg* dlg = EditRecordDlg::ForUpdate(this, m_db, record_id);
    dlg->ShowModal();
    dlg->Destroy();
}
