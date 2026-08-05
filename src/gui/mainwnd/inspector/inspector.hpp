#pragma once
#include <wx/event.h>
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/mainwnd/treectrl/treectrl.hpp"
#include "gui/mainwnd/mainwnd.hpp" // <- 必要

enum class Status {
	HIDE,
	SHOW,
};

class Inspector : public wxPanel {
public:
	Inspector(wxWindow* parent, Database& db);
	void UpdateSelectedCategory(int id, const wxString& name);
	void ViewCtrl(Status);
	void OnEnterPressed(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
private:
	Database &m_db;
	int m_selected_id;
	wxString m_selected_name;
	wxStaticText* m_st_label_ID;
	wxStaticText* m_st_label_ID_num;
	wxStaticText* m_st_label_name;
	wxTextCtrl* m_tc_name;
	wxStaticText* m_st_path;
	wxStaticText* m_label_path;

	wxString m_old_name; // 名称アプデ用。前回の値を保持
	CategoryTree* m_categoryTree = nullptr;
};

