#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include "core/db/database.hpp"
#include "gui/mainwnd/treectrl/treectrl.hpp"

class ActivityReport;
class RecordViewer;
class ToDoPanel;
class GoalTab;

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

	// Notebook の表示期間 (チェックなし = 全期間。Goal タブだけは今の窓)
	wxCheckBox* m_cb_filter;
	wxDatePickerCtrl* m_dp_from;
	wxDatePickerCtrl* m_dp_to;
	ActivityReport* m_activity_report; // Activity Report タブ。この Category 以下の集計
	RecordViewer* m_record_viewer; // Record タブ。この Category 以下の record
	ToDoPanel* m_todo_panel;       // ToDo タブ。この Category 以下の ToDo (ToDo ウィンドウと共通の部品)
	GoalTab* m_goal_tab;           // Goal タブ。この Category に設定された goal の窓

	void RefreshTabs();            // 期間の設定に合わせて、各タブを更新する
	void OnSetTitle(wxCommandEvent& event);
	void OnSetTextOfDetail(wxCommandEvent& event);
};
