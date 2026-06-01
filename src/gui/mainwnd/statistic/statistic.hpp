#pragma once
#include <wx/event.h>
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "core/utils/format_time.hpp"


class Statistic : public wxPanel {
public:
	Statistic(wxWindow* parent, Database& db);
	void OnUpdateStatistic();
	void GetId(int id);
	void GetCurrentRange(const wxDateTime& start, const wxDateTime& end);
private:
	Database &m_db;
	int m_selected_id;
	wxStaticText* m_result_total_time_all;
	wxStaticText* m_result_total_time_range;
	wxStaticText* m_last_executed;
	wxDateTime m_current_start;
	wxDateTime m_current_end;
};

