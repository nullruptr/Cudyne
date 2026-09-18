#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/mainwnd/treectrl/treectrl.hpp"

class Detail : public wxFrame{
public:
	Detail(wxWindow* parent, Database &dbRef, int id);
private:
    Database &m_db;
    int m_id;
	CategoryTree* m_categoryTree;
};
