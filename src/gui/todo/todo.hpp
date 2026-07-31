#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/timer.h>
#include <vector>
#include "core/db/database.hpp"

class ToDo : public wxFrame{

public:
	ToDo(wxWindow* parent, Database& db);
private:
	Database &m_db;
	wxListCtrl* m_list;
	wxChoice* m_choiceCondition;
	wxDatePickerCtrl* m_dp_filter_date; // この日付以降の締切のものを表示する(超過分は常に表示)
	wxButton* m_btn_add;
	wxButton* m_btn_edit;
	wxTimer m_timer; // 締切等までのカウントダウンをリアルタイム更新する

	std::vector<Database::ToDo> m_todo_cache; // 現在表示中の内容。m_list の行と添字が対応する
	int m_sort_column = -1;
	bool m_sort_ascending = true;

	void RefreshList(); // DB へ再取得しに行き、フィルタ・ソートをかけて再描画する
	void RenderFromCache(); // m_todo_cache の内容で m_list を再構築する(DB へは行かない)
	void ApplySort(); // m_sort_column/m_sort_ascending に従い m_todo_cache をソートする
	wxColour ComputeRowColor(const Database::ToDo& todo, long long now_epoch) const;

	void OnConditionChanged(wxCommandEvent& WXUNUSED(event));
	void OnFilterDateChanged(wxDateEvent& WXUNUSED(event));
	void OnAdd(wxCommandEvent& WXUNUSED(event));
	void OnEdit(wxCommandEvent& WXUNUSED(event));
	void OnListDoubleClick(wxListEvent& WXUNUSED(event));
	void OnColumnClick(wxListEvent& event);
	void OnTimer(wxTimerEvent& WXUNUSED(event));
};
