#pragma once

#include <wx/wx.h>
#include <gui/mainwnd/treectrl/treectrl.hpp>
#include "core/db/database.hpp"

class SelCategoryDlg : public wxDialog {
public:
    // allow_folder: true のとき、OK ボタンでフォルダも選択できる
    SelCategoryDlg(wxWindow* parent, Database& db, bool allow_folder = false);
    int GetSelectedCategoryId() const { return m_selected_id; }
    wxString GetSelectedCategoryName() const { return m_selected_name; }
private:
    CategoryTree* m_tree;
    Database& m_db;
    bool m_allow_folder;

    int m_selected_id = -1;
    wxString m_selected_name;

    void TryConfirm(bool allow_folder); // 選択中の項目を確定して閉じる
};
