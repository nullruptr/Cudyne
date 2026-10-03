#include "goal_list_dlg.hpp"
#include "edit_goal_dlg.hpp"
#include "core/utils/format_time.hpp"

namespace {
wxString PeriodText(const Database::Goal& g) {
    switch (g.period_type) {
    case 0: return _("Daily");
    case 1: return _("Weekly");
    case 2: return _("Monthly");
    case 3: return wxString::Format(_("Every %d Days"), g.period_n);
    case 4: return _("Undefined");
    default: return "-";
    }
}
}

GoalListDlg::GoalListDlg(wxWindow* parent, Database& dbRef, int category_id, int todo_id)
    : wxDialog(parent, wxID_ANY, _("Goals"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef)
    , m_category_id(category_id)
    , m_todo_id(todo_id) {

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(760, 400)));

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->InsertColumn(0, _("Name"),   wxLIST_FORMAT_LEFT, FromDIP(140));
    m_list->InsertColumn(1, _("Period"), wxLIST_FORMAT_LEFT, FromDIP(100));
    m_list->InsertColumn(2, _("Target"), wxLIST_FORMAT_LEFT, FromDIP(90));
    m_list->InsertColumn(3, _("Start"),  wxLIST_FORMAT_LEFT, FromDIP(140));
    m_list->InsertColumn(4, _("End"),    wxLIST_FORMAT_LEFT, FromDIP(140));
    m_list->InsertColumn(5, _("Active"), wxLIST_FORMAT_LEFT, FromDIP(60));

    wxButton* btn_new = new wxButton(this, wxID_ANY, _("New"));
    m_btn_edit = new wxButton(this, wxID_ANY, _("Edit"));
    m_btn_delete = new wxButton(this, wxID_ANY, _("Delete"));
    wxButton* btn_close = new wxButton(this, wxID_ANY, _("Close"));

    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(btn_new, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(m_btn_edit, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(m_btn_delete, 0, wxALL, FromDIP(5));
    bottom_sizer->AddStretchSpacer(1);
    bottom_sizer->Add(btn_close, 0, wxALL, FromDIP(5));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, FromDIP(5));
    sizer->Add(bottom_sizer, 0, wxEXPAND);
    SetSizer(sizer);
    CenterOnParent();

    Reload();

    btn_new->Bind(wxEVT_BUTTON, &GoalListDlg::OnNew, this);
    m_btn_edit->Bind(wxEVT_BUTTON, &GoalListDlg::OnEdit, this);
    m_btn_delete->Bind(wxEVT_BUTTON, &GoalListDlg::OnDelete, this);
    btn_close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CLOSE); });

    // ダブルクリックで編集
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent&) {
        wxCommandEvent evt;
        OnEdit(evt);
    });
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED,   [this](wxListEvent&) { UpdateButtons(); });
    m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { UpdateButtons(); });
}

void GoalListDlg::Reload() {
    m_list->DeleteAllItems();
    m_goal_cache = m_db.GetGoalsByTarget(m_category_id, m_todo_id);

    for (size_t i = 0; i < m_goal_cache.size(); ++i) {
        const Database::Goal& g = m_goal_cache[i];

        // 名前は任意なので、空のときは代わりの表示にする
        long idx = m_list->InsertItem((long)i, g.goal_name.empty() ? wxString(_("(no name)")) : wxString::FromUTF8(g.goal_name));
        m_list->SetItem(idx, 1, PeriodText(g));
        m_list->SetItem(idx, 2, TimeUtils::FormatSeconds(g.target_time));
        m_list->SetItem(idx, 3, TimeUtils::FormatUTCStringToLocal(g.start_time));
        m_list->SetItem(idx, 4, g.end_time.empty() ? wxString("-") : TimeUtils::FormatUTCStringToLocal(g.end_time));
        m_list->SetItem(idx, 5, g.is_active == 1 ? "Yes" : "No");
    }
    UpdateButtons();
}

void GoalListDlg::UpdateButtons() {
    const bool selected = GetSelectedIndex() != -1;
    m_btn_edit->Enable(selected);
    m_btn_delete->Enable(selected);
}

long GoalListDlg::GetSelectedIndex() const {
    return m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void GoalListDlg::OnNew(wxCommandEvent& WXUNUSED(event)) {
    EditGoalDlg dlg(this, m_db, m_category_id, m_todo_id);
    dlg.ShowModal();
    Reload();
}

void GoalListDlg::OnEdit(wxCommandEvent& WXUNUSED(event)) {
    long idx = GetSelectedIndex();
    if (idx < 0 || (size_t)idx >= m_goal_cache.size()) return;

    EditGoalDlg dlg(this, m_db, -1, -1, m_goal_cache[(size_t)idx].goal_id);
    dlg.ShowModal();
    Reload();
}

void GoalListDlg::OnDelete(wxCommandEvent& WXUNUSED(event)) {
    long idx = GetSelectedIndex();
    if (idx < 0 || (size_t)idx >= m_goal_cache.size()) return;

    if (wxMessageBox(_("Delete the selected goal?"), _("Confirm"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES) {
        return;
    }
    if (!m_db.DeleteGoal(m_goal_cache[(size_t)idx].goal_id)) {
        wxMessageBox(_("Failed to delete"), "Error", wxOK | wxICON_ERROR);
        return;
    }
    Reload();
}
