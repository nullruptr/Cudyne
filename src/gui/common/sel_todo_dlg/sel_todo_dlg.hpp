#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "core/db/database.hpp"

class SelToDoDlg : public wxDialog {
public:
    SelToDoDlg(wxWindow* parent, Database& db);
    int GetSelectedTodoId() const { return m_selected_id; }
    wxString GetSelectedTodoName() const { return m_selected_name; }
private:
    Database& m_db;
    wxListCtrl* m_list;
    std::vector<Database::ToDo> m_todo_cache; // m_list の行と添字が対応する

    int m_selected_id = -1;
    wxString m_selected_name;

    void OnItemActivated(wxListEvent& event);
};
