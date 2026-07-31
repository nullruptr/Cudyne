#pragma once
#include <wx/wx.h>
#include <wx/dataview.h>
#include "core/db/database.hpp"

class ToDo : public wxFrame{

public:
	ToDo(wxWindow* parent, Database& db);
private:
	Database &m_db;
	wxDataViewListCtrl* m_dvlc;
    wxChoice* m_choiceCondition;
	wxButton* m_btn_add;
	wxButton* m_btn_edit;

	void RefreshList(); // m_choiceCondition の選択状態に応じて一覧を再取得・再描画
	void OnConditionChanged(wxCommandEvent& WXUNUSED(event));
};
