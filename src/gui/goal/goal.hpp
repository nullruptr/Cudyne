#pragma once
#include <wx/event.h>
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/timer.h>
#include <wx/dataview.h>
#include "core/db/database.hpp"

// 列
enum GoalColumn {
    Col_Name,
    Col_Period,
    Col_Target,
    Col_Start,
    Col_End,
    Col_Active,
    Col_Count // 列の数(GetColumnCount で返す)
};

class Goal : public wxFrame{
public:
	Goal(wxWindow* parent, Database& db);
private:
	Database &m_db;
	wxChoice* m_choiceCondition;
	wxDatePickerCtrl* m_dp_filter_date; // この日付以降の締切のものを表示する(超過分は常に表示)
	wxButton* m_btn_add;
	wxButton* m_btn_edit;
	wxTimer m_timer; // 締切等までのカウントダウンをリアルタイム更新する

    wxDataViewCtrl* m_ctrl;
    void OnTimer(wxTimerEvent& WXUNUSED(event));
};
