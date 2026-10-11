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
	wxStaticText* m_title;
	wxStaticText* m_result_total_time_incl_subitems;
	wxStaticText* m_result_total_time_selected;
	wxStaticText* m_last_executed;

	// Goal の要約 (この Category に設定された goal の件数)
	wxStaticText* m_goal_total;
	wxStaticText* m_goal_achieved;
	wxStaticText* m_goal_unachieved;
	wxStaticText* m_goal_invalid;

	void OnSetTitle(wxCommandEvent& event);
	void OnSetTextOfDetail(wxCommandEvent& event);
};
