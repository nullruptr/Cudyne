#include "sel_category_dlg.hpp"
#include "gui/time_log/tree_item_data.hpp"


SelCategoryDlg::SelCategoryDlg(wxWindow* parent, Database &dbRef)
    : wxDialog(parent, wxID_ANY, _("Select Category"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) {
    m_tree = new CategoryTree(this, m_db);
    m_tree->UpdateTreeData();

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_tree, 1, wxEXPAND | wxALL, FromDIP(5));
    SetSizer(sizer);

    // ダブルクリックで選択確定
    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, [this](wxTreeEvent& e) {
        wxTreeItemId item = m_tree->GetSelection();
        TreeItemData* data = (TreeItemData*)m_tree->GetItemData(item);
        if (!data) return;
        if (m_db.IsFolder(data->GetId())) return;
        m_selected_id = data->GetId();
        m_selected_name = m_tree->GetItemText(item);
        EndModal(wxID_OK);
    });
    }
