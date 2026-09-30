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
	wxStaticText* m_period_type;
	wxStaticText* m_goal;
	wxStaticText* m_remaining;
	wxStaticText* m_deadline;
	wxStaticText* m_time_to_deadline;

	void OnSetTitle(wxCommandEvent& event);
	void OnSetTextOfDetail(wxCommandEvent& event);
};
