#include "sel_category_dlg.hpp"
#include "gui/time_log/tree_item_data.hpp"


SelCategoryDlg::SelCategoryDlg(wxWindow* parent, Database &dbRef, bool allow_folder)
    : wxDialog(parent, wxID_ANY, _("Select Category"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef)
    , m_allow_folder(allow_folder) {
    m_tree = new CategoryTree(this, m_db);
    m_tree->UpdateTreeData();

    wxButton* btn_ok = new wxButton(this, wxID_ANY, _("OK"));
    wxButton* btn_cancel = new wxButton(this, wxID_ANY, _("Cancel"));

    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->AddStretchSpacer(1);
    bottom_sizer->Add(btn_ok, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(btn_cancel, 0, wxALL, FromDIP(5));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tree, 1, wxEXPAND | wxALL, FromDIP(5));
    sizer->Add(bottom_sizer, 0, wxEXPAND);
    SetSizer(sizer);

    // ダブルクリックで選択確定 (フォルダはダブルクリックでは選択できない)
    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, [this](wxTreeEvent& e) {
        TryConfirm(false);
    });

    // OK ボタンでは、allow_folder が true ならフォルダも選択できる
    btn_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { TryConfirm(m_allow_folder); });
    btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void SelCategoryDlg::TryConfirm(bool allow_folder) {
    wxTreeItemId item = m_tree->GetSelection();
    if (!item.IsOk()) return;
    TreeItemData* data = (TreeItemData*)m_tree->GetItemData(item);
    if (!data) return;
    if (!allow_folder && m_db.IsFolder(data->GetId())) return;
    m_selected_id = data->GetId();
    m_selected_name = m_tree->GetItemText(item);
    EndModal(wxID_OK);
}
