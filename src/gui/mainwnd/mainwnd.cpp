#include <wx/event.h>
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/treectrl.h>
#include <wx/regex.h>
#include "mainwnd.hpp"
#include "gui/mainwnd/activity_report/activity_report.hpp"
#include "gui/mainwnd/controller/controller.hpp"
#include "gui/mainwnd/record_viewer/record_viewer.hpp"
#include "gui/time_log/time_log.hpp"
#include "gui/connect_db/connect_db.hpp"
#include "gui/mainwnd/treectrl/treectrl.hpp"

Mainwnd::Mainwnd(wxWindow* parent) : wxFrame(parent, wxID_ANY, _("wxAUI Test"),
	wxDefaultPosition) {
	
	/*
	wxLog::SetLogLevel(wxLOG_Max);
	wxLog::SetActiveTarget(new wxLogWindow(this, "Debug Log", true));
	wxLogMessage("Log System Initialized.");
	*/
        // notify wxAUI which frame to use
	SetSize(FromDIP(wxSize(1600, 800)));
        m_mgr.SetManagedWindow(this);

	// メニュー内容の設定
	wxMenu* menuFile = new wxMenu;
	menuFile->Append(ID_CONNECT_DB, _("Connect DB"));
	menuFile->Append(ID_SETTINGS, _("Settings (Not implemented)"));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT, _("Quit\t F12"));

	wxMenu* menuEdit = new wxMenu;
	menuEdit->Append(ID_MENU_TODO, _("ToDo"));

	wxMenu *menuLegacy = new wxMenu;
	menuLegacy->Append(ID_TIME_LOG, _("Time Log"));

	// メニューバーの設定
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _("File"));
	menuBar->Append(menuEdit, _("Edit"));
	menuBar->Append(menuLegacy, _("Legacy"));
	SetMenuBar(menuBar);

	// メニューバインド
	Bind(wxEVT_MENU, &Mainwnd::OnConnectDB, this, ID_CONNECT_DB);
	Bind(wxEVT_MENU, &Mainwnd::OnQuit, this, wxID_EXIT);
	Bind(wxEVT_MENU, &Mainwnd::OnTodo, this, ID_MENU_TODO);
	Bind(wxEVT_MENU, &Mainwnd::OnTimeLog, this, ID_TIME_LOG);

	// 発火イベント受信用
	Bind(wxEVT_COMMAND_MENU_SELECTED, &Mainwnd::OnCategorySelected, this, ID_CATEGORY_SELECTED);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &Mainwnd::OnStartRecordToRecWnd, this, ID_START_RECORDING);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &Mainwnd::OnRecordUpdate, this, ID_UPDATE_STATISTIC);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &Mainwnd::OnRecordUpdate, this, ID_ACTIVITY_REPORT);
	Bind(wxEVT_COMMAND_MENU_SELECTED, &Mainwnd::OnCategoryUpdated, this, ID_CATEGORY_UPDATED);

	// Dashboard
	m_dashboard = new Dashboard(this);
	// Controller
	m_controller = new Controller(this, db);
	// Clock パネルの生成
	m_clock = new Clock(this);
	// Recording
	m_recording = new Recording(this, db);
	// TreeCtrl
	m_categoryTree = new CategoryTree(this, db);
	// Activity Report
	m_activity_report = new ActivityReport(this, db);
	// Record Viewer
	m_record_viewer = new RecordViewer(this, db);
	// Inspector
	m_inspector = new Inspector(this, db);
	// Statistic
	m_statistic = new Statistic(this, db);

	m_mgr.AddPane(m_dashboard, wxAuiPaneInfo().CenterPane());

	// AuiNotebook
	m_right_notebook = new wxAuiNotebook(this, wxID_ANY,
					     wxDefaultPosition, wxDefaultSize,
					     wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE);
	// Notebook に追加
	m_right_notebook->AddPage(m_activity_report, _("Activity Report"));
	m_right_notebook->AddPage(m_record_viewer,   _("Record Viewer"));

	// Notebook を aui に登録
	m_mgr.AddPane(m_right_notebook, wxAuiPaneInfo()
	    .Right()
	    .Caption(_("Reports"))
	    .Name(wxT("RightNotebook"))
	    .BestSize(FromDIP(400), -1)
	    .Layer(1)
	    .CloseButton(false)
	);

	m_mgr.AddPane(m_clock, wxAuiPaneInfo()
	    .Name(wxT("clockPane"))      // 内部識別名
	    .Caption(_("Clock"))         // キャプション
	    .Top()                       // 画面上部に配置
	    .Layer(1)                    // レイヤー設定（他のペインとの前後関係）
	    .PaneBorder(false)           // 境界線を消す
	    .CaptionVisible(false)       // 時計にタイトルバーは不要なので false
	    .CloseButton(false)          // 閉じられないように設定
	    .DockFixed()                 // 位置を固定したい場合
	    .BestSize(-1, FromDIP(33))            // 高さを 40px 程度に固定
	);
	
	m_mgr.AddPane(m_recording, wxAuiPaneInfo()
        .Bottom()
        .Caption(_("Recording"))
        .Name(wxT("Recording_wnd"))
        .BestSize(FromDIP(333), FromDIP(333))
        .Layer(0)
	.Position(0)
	.PaneBorder(true)
	.CloseButton(false) // 閉じるボタン無効
	); 

	m_mgr.AddPane(m_categoryTree, wxAuiPaneInfo()
        .Left()
        .Caption(_("Categories"))
        .Name(wxT("treePane"))
        .BestSize(FromDIP(166), -1)
        .Layer(1)
	.Row(0) // 左側のエリアの 0番目
	.Position(0)
	.CloseButton(false) // 閉じるボタン無効
	);

	m_mgr.AddPane(m_controller, wxAuiPaneInfo()
        .Left()
        .Caption(_("Controller"))
        .Name(wxT("Controller"))
        .BestSize(FromDIP(300), -1)
        .Layer(1)
	.Row(1) // 左側のエリアの 1番目
	.Position(0)
	.CloseButton(false) // 閉じるボタン無効
	);

	m_mgr.AddPane(m_inspector, wxAuiPaneInfo()
        .Left()
        .Caption(_("Inspector"))
        .Name(wxT("Inspector"))
        .BestSize(FromDIP(300), -1)
        .Layer(1)
	.Row(1) // 左側のエリアの 1番目
	.Position(1)
	.CloseButton(false) // 閉じるボタン無効
	);

	m_mgr.AddPane(m_statistic, wxAuiPaneInfo()
        .Left()
        .Caption(_("Statistic"))
        .Name(wxT("Statistic"))
        .BestSize(FromDIP(300), -1)
        .Layer(1)
	.Row(1) // 左側のエリアの 1番目
	.Position(2)
	.CloseButton(false) // 閉じるボタン無効
	); 

	// ステータスバーの設定
	CreateStatusBar(2);
	// 初期表示
	SetStatusText(_("Disconnected"), 0);
	SetStatusText("", 1);

	// F12 で終了処理
	wxAcceleratorEntry entry;
	entry.Set(wxACCEL_NORMAL, WXK_F12, wxID_EXIT);
	SetAcceleratorTable(wxAcceleratorTable(1, &entry));
	
	CenterOnScreen(); // 画面真ん中に表示
	
        // tell the manager to "commit" all the changes just made
        m_mgr.Update();
	// 高DPI対応
	Bind(wxEVT_DPI_CHANGED, &Mainwnd::OnDPIChanged, this);
    }

void Mainwnd::OnQuit(wxCommandEvent& WXUNUSED(event)){ // 終了確認
	int btn_status = wxMessageBox(_("Are you sure you want to quit?"),
			wxT("MB"),
			wxOK| wxCANCEL | wxICON_INFORMATION,
			this);

	if (btn_status != wxOK){ // OK以外が押されたら何も処理しない
		return;
	}

	Close(true);
}

void Mainwnd::OnTodo(wxCommandEvent& event) {
	wxLogDebug("OnTodo called");
	// DB 接続判定
	if (current_DB_Path.IsEmpty()){
		wxMessageBox(
				_("Unable to connect database"),
				"DB Error",
				wxOK | wxICON_ERROR,
				this
			    );
		return;
	}
	wxLogDebug("DB check passed");
	Todo* todo = new Todo(this, db);
	todo->Show(true);

}

void Mainwnd::OnTimeLog(wxCommandEvent& event){
	// DB 接続判定
	if (current_DB_Path.IsEmpty()){
		wxMessageBox(
				_("Unable to connect database"),
				"DB Error",
				wxOK | wxICON_ERROR,
				this
			    );
		return;
	}
	TimeLog* log = new TimeLog(this, db, current_DB_Path);
	log->Show(true);
}

void Mainwnd::OnConnectDB(wxCommandEvent& event){
	ConnectDB dlg(this); // 新規ダイアログ作成

	if (dlg.ShowModal() == wxID_OK){ // モーダル表示。ファイル確認完了後、接続処理へ。
		std::string path = std::string(dlg.GetPath().ToUTF8());

		if (db.Connect(path)){
			// ステータスバーに設定
			current_DB_Path = dlg.GetPath();
			SetStatusText(_("Connected"), 0);
			SetStatusText(current_DB_Path, 1);
			m_categoryTree->UpdateTreeData();
			wxCommandEvent dummy;
			OnRecordUpdate(dummy);
		} else {
			wxMessageBox(_("Connection failed"));
		}
	}
}

void Mainwnd::OnDPIChanged(wxDPIChangedEvent& event) {
    if (m_dpiChanging) {
        event.Skip();
        return;
    }

    const wxSize oldDPI = event.GetOldDPI();
    const wxSize newDPI = event.GetNewDPI();

    if (oldDPI.x == newDPI.x) {
        event.Skip();
        return;
    }

    m_dpiChanging = true;
    wxLogDebug("=== DPI Changed === %d -> %d", oldDPI.x, newDPI.x);

    // 現在の実サイズから best_size を再取得
    wxAuiPaneInfoArray panes = m_mgr.GetAllPanes();
    for (size_t i = 0; i < panes.GetCount(); i++) {
    wxAuiPaneInfo& pinfo = panes.Item(i);
    if (!pinfo.window) continue;
    wxAuiPaneInfo& p = m_mgr.GetPane(pinfo.name);
    if (!p.IsOk()) continue;
    p.best_size = pinfo.window->GetSize();
    p.best_size.IncTo(pinfo.window->GetBestSize());
    p.best_size.IncTo(pinfo.min_size);
    wxLogDebug("UPDATE [%s] best=(%d,%d)",
        p.name, p.best_size.x, p.best_size.y);
}

    // dock_size も Perspective 経由でスケーリング
    const double scale = (double)newDPI.x / oldDPI.x;
    wxString perspective = m_mgr.SavePerspective();
    wxRegEx re("dock_size\\(([^)]+)\\)=([0-9]+)");
    wxString result;
    wxString src = perspective;
    while (!src.IsEmpty()) {
        size_t start, len;
        if (re.Matches(src) && re.GetMatch(&start, &len, 0)) {
            result += src.Left(start);
            wxString args = re.GetMatch(src, 1);
            long oldVal;
            re.GetMatch(src, 2).ToLong(&oldVal);
            result += wxString::Format("dock_size(%s)=%ld", args, (long)(oldVal * scale));
            src = src.Mid(start + len);
        } else {
            result += src;
            break;
        }
    }

    m_mgr.LoadPerspective(result, true);

    m_dpiChanging = false;
    event.Skip();
}

// --- 以下イベント転送 ---

void Mainwnd::OnCategorySelected(wxCommandEvent& event) { // 表示内容更新など
	// イベントに格納された数値を引っ張り出す
	int catId = event.GetInt();
	wxString catName = event.GetString();

	// inspector と statistic の表示を更新
	if (m_inspector) {
		m_inspector->UpdateSelectedCategory(catId, catName);
		m_statistic->GetId(catId);
	}
	if (m_activity_report && m_statistic) {
		// 選択区間を送付する 
		wxCommandEvent dummy;
		OnRecordUpdate(dummy);
	}
}

void Mainwnd::OnStartRecordToRecWnd(wxCommandEvent& event) {
	int catId = event.GetInt();
	wxString catName = event.GetString();

	// イベントに値がない場合（▶ボタン経由）は Inspector から取得
	if (catId == 0 && catName.IsEmpty()) {
		m_inspector->GetIdAndName(catId, catName);
	}

	if (m_recording) {
		m_recording->OnStartRecord(catId, catName);
	}
}

void Mainwnd::OnRecordUpdate(wxCommandEvent& event) { // 表示内容更新 & start time と end time を取得
	if (m_activity_report && m_controller && m_statistic) {
		wxDateTime start, end;
		m_controller->GetCurrentRange(start, end);
		m_activity_report->SetPeriodAndRefresh(start, end);
		m_record_viewer->SetPeriodAndRefresh(start, end);
		m_statistic->GetCurrentRange(start, end);
	}
}

void Mainwnd::OnCategoryUpdated(wxCommandEvent& event) {
	if (m_categoryTree) {
		m_categoryTree->UpdateTreeData();
	}
}

Mainwnd::~Mainwnd(){
        // deinitialize the frame manager
        m_mgr.UnInit();
}
