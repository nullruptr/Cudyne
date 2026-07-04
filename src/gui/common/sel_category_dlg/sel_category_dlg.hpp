#pragma once

#include <wx/wx.h>
#include <gui/mainwnd/treectrl/treectrl.hpp>
#include "core/db/database.hpp"

class SelCategoryDlg : public wxDialog {
public:
    SelCategoryDlg(wxWindow* parent, Database& db);
    int GetSelectedCategoryId() const { return m_selected_id; }
    wxString GetSelectedCategoryName() const { return m_selected_name; }
private:
    CategoryTree* m_tree;
    Database& m_db;

    int m_selected_id = -1;
    wxString m_selected_name;
};
