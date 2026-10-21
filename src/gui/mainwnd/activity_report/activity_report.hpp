#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include "core/db/database.hpp"

class ActivityReport : public wxPanel {
public:
    ActivityReport(wxWindow* parent, Database& dbRef);
    void SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end);
    void SetCategoryId(int category_id); // -1 = すべて。指定すると、そのカテゴリ以下の集計だけを表示する

private:
	void LoadReport();

	Database& m_db;
	wxListCtrl* m_list;
	wxStaticText* m_total_label;
	wxDateTime m_current_start;
	wxDateTime m_current_end;
	int m_category_id = -1;
};
