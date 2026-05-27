#pragma once
#include <wx/math.h>
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/treebase.h>
#include <wx/treectrl.h>
#include "core/db/database.hpp"
#include "gui/mainwnd/treectrl/treectrl.hpp"
#include "gui/mainwnd/clock/clock.hpp"
#include "gui/mainwnd/recording/recording.hpp"
#include "gui/mainwnd/dashboard/dashboard.hpp"
#include "gui/mainwnd/controller/controller.hpp"
#include "gui/mainwnd/activity_report/activity_report.hpp"
#include "gui/mainwnd/inspector/inspector.hpp"
#include "gui/mainwnd/statistic/statistic.hpp"
#include "gui/todo/todo.hpp"

enum {
	ID_CONNECT_DB, //DBファイル
	ID_SETTINGS, //設定ウィンドウ
	ID_MENU_TODO, //TODO ウィンドウ
	ID_TIME_LOG, //時間記録画面
	ID_ACTIVITY_REPORT, // 実績照会ウィンドウ表示
	ID_CATEGORY_SELECTED = wxID_HIGHEST + 1, // TreeCtrl からの情報受取
	ID_START_RECORDING, // レコード開始イベント監視
	ID_UPDATE_STATISTIC, // 統計更新イベント処理
	ID_CATEGORY_UPDATED // カテゴリ名が更新されたとき
};

class Inspector;

class Mainwnd : public wxFrame {
public:
Mainwnd(wxWindow* parent);
	void OnTimer(wxTimerEvent &event); // 時間表示をリアルタイムで行うために必要
	void OnQuit(wxCommandEvent &event); // 終了インベント処理
	void OnTodo(wxCommandEvent &event); // 終了インベント処理
	void OnTimeLog(wxCommandEvent &event); // 時間記録画面を開く
	void OnConnectDB(wxCommandEvent &event);
	void OnDPIChanged(wxDPIChangedEvent& event);
	Database db;
	wxString current_DB_Path;
	void OnCategorySelected(wxCommandEvent& event); // Controller のID と CatName の表示更新。Controllerに渡す
	void OnStartRecordToRecWnd(wxCommandEvent& event);
	void OnRecordUpdate(wxCommandEvent& event);
	void OnCategoryUpdated(wxCommandEvent& event);

virtual ~Mainwnd();

private:
	wxAuiManager m_mgr;
	CategoryTree* m_categoryTree;
	Clock* m_clock;
	Recording* m_recording;
	Dashboard* m_dashboard;
	Controller* m_controller;
	ActivityReport* m_activity_report;
	Inspector* m_inspector;
	Statistic* m_statistic;
	bool m_dpiChanging = false;
};
