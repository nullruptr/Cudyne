#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/stringimpl.h>
#include <wx/versioninfo.h>
#include <wx/wx.h>
#include <wx/notebook.h>
#include "detail.hpp"
#include "core/utils/format_time.hpp"
#include "core/utils/utils.hpp"
#include "core/goal/goal_status.hpp"
#include "gui/goal/goal_list_dlg.hpp"
#include "gui/goal/goal_tab.hpp"
#include "gui/mainwnd/activity_report/activity_report.hpp"
#include "gui/todo/todo_panel.hpp"
#include "gui/mainwnd/mainwnd.hpp"
#include "gui/mainwnd/record_viewer/record_viewer.hpp"
#include <wx/dateevt.h>
#include <ctime>
#include <vector>

Detail::Detail(wxWindow* parent, Database &dbRef, int id)
	: wxFrame(parent, wxID_ANY, wxT("Detail"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
      , m_db(dbRef)
      , m_id(id) {
    
    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(600, 600)));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL); // メインサイザー
	wxPanel* pnlDetail = new wxPanel(this, wxID_ANY);
	mainSizer->Add(pnlDetail, 1, wxEXPAND, 0);

    wxBoxSizer* rightBoxSizer = new wxBoxSizer(wxVERTICAL);
    m_title = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
    rightBoxSizer->Add(m_title, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);
	// 2列 (ラベル / 値)、行数は項目数に応じて自動
	wxFlexGridSizer* flex = new wxFlexGridSizer(0, 2, FromDIP(8), FromDIP(20));
	flex->AddGrowableCol(1, 1); // 値の列を伸縮させる

	wxStaticText* stat_total_time_incl_subitems = new wxStaticText(pnlDetail, wxID_ANY, _("Total Time (Including Subitems):"));
	m_result_total_time_incl_subitems = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_total_time_selected = new wxStaticText(pnlDetail, wxID_ANY, _("Total Time (Selected Category):"));
	m_result_total_time_selected = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_last_executed = new wxStaticText(pnlDetail, wxID_ANY, _("Last Executed:"));
	m_last_executed = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_goal_total = new wxStaticText(pnlDetail, wxID_ANY, _("Goals"));
	m_goal_total = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_goal_achieved = new wxStaticText(pnlDetail, wxID_ANY, _("Goals Achieved"));
	m_goal_achieved = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_goal_unachieved = new wxStaticText(pnlDetail, wxID_ANY, _("Goals Unachieved"));
	m_goal_unachieved = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_goal_invalid = new wxStaticText(pnlDetail, wxID_ANY, _("Goals Invalid"));
	m_goal_invalid = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);

	// ラベルと値をペアで登録
	const wxSizerFlags labelFlags = wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL);
	const wxSizerFlags valueFlags = wxSizerFlags(1).Expand().Align(wxALIGN_CENTER_VERTICAL);
	flex->Add(stat_total_time_incl_subitems, labelFlags);
	flex->Add(m_result_total_time_incl_subitems, valueFlags);
	flex->Add(stat_total_time_selected, labelFlags);
	flex->Add(m_result_total_time_selected, valueFlags);
	flex->Add(stat_last_executed, labelFlags);
	flex->Add(m_last_executed, valueFlags);
	flex->Add(stat_goal_total, labelFlags);
	flex->Add(m_goal_total, valueFlags);
	flex->Add(stat_goal_achieved, labelFlags);
	flex->Add(m_goal_achieved, valueFlags);
	flex->Add(stat_goal_unachieved, labelFlags);
	flex->Add(m_goal_unachieved, valueFlags);
	flex->Add(stat_goal_invalid, labelFlags);
	flex->Add(m_goal_invalid, valueFlags);

	rightBoxSizer->Add(flex, 0, wxEXPAND | wxALL, FromDIP(10));

	wxButton* btn_setup_goal = new wxButton(pnlDetail, wxID_ANY, _("Setup Goal"));
	wxButton* btn_setup_plan = new wxButton(pnlDetail, wxID_ANY, _("Setup Plan"));

	btn_setup_goal->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		// 同じ対象に複数の goal を持てるので、一覧から選んで編集・新規作成する
		GoalListDlg dlg(this, m_db, m_id);
		dlg.ShowModal();

		// goal の追加・編集・削除を要約に反映する
		wxCommandEvent evt;
		OnSetTextOfDetail(evt);
		RefreshTabs(); // Goal タブにも反映する
	});

	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->Add(btn_setup_goal, 0, wxLEFT, FromDIP(8));
	btnSizer->Add(btn_setup_plan, 0);
	rightBoxSizer->Add(btnSizer, 0, wxALL, FromDIP(10));

	// Notebook に表示する期間 (チェックを外すと全期間)
	const long date_picker_style = wxDP_DROPDOWN | wxDP_SHOWCENTURY; // カレンダーから選べる
	m_cb_filter = new wxCheckBox(pnlDetail, wxID_ANY, _("Filter by period"));
	wxDateTime first_day = wxDateTime::Today();
	first_day.SetDay(1);
	wxDateTime last_day = first_day;
	last_day.SetToLastMonthDay();
	m_dp_from = new wxDatePickerCtrl(pnlDetail, wxID_ANY, first_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
	m_dp_to   = new wxDatePickerCtrl(pnlDetail, wxID_ANY, last_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
	m_dp_from->Enable(false);
	m_dp_to->Enable(false);
	wxBoxSizer* filterSizer = new wxBoxSizer(wxHORIZONTAL);
	filterSizer->Add(m_cb_filter, 0, wxALIGN_CENTER_VERTICAL);
	filterSizer->Add(m_dp_from, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
	filterSizer->Add(new wxStaticText(pnlDetail, wxID_ANY, wxT("~")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(5));
	filterSizer->Add(m_dp_to, 0, wxALIGN_CENTER_VERTICAL);
	rightBoxSizer->Add(filterSizer, 0, wxLEFT | wxRIGHT, FromDIP(10));

	wxNotebook* notebook = new wxNotebook(pnlDetail, wxID_ANY);
	m_activity_report = new ActivityReport(notebook, m_db);
	m_activity_report->SetCategoryId(m_id); // この Category 以下の集計に絞る
	notebook->AddPage(m_activity_report, _("Activity Report"));
	m_record_viewer = new RecordViewer(notebook, m_db);
	m_record_viewer->SetCategoryId(m_id); // この Category 以下の record に絞る
	notebook->AddPage(m_record_viewer, _("Record"));
	// 期間は Detail の「Filter by period」で決める。「Start Recording」は、メインウィンドウ側へ送る
	m_todo_panel = new ToDoPanel(notebook, m_db, m_id, true, GetParent());
	notebook->AddPage(m_todo_panel, _("ToDo"));
	m_goal_tab = new GoalTab(notebook, m_db, m_id);
	notebook->AddPage(m_goal_tab, _("Goal"));
	notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Plan"));
	rightBoxSizer->Add(notebook, 1, wxEXPAND | wxALL, FromDIP(10));

	m_cb_filter->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		m_dp_from->Enable(m_cb_filter->GetValue());
		m_dp_to->Enable(m_cb_filter->GetValue());
		RefreshTabs();
	});
	// From > To にならないよう、動かした側に合わせてもう一方を寄せる
	m_dp_from->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
		if (m_dp_from->GetValue() > m_dp_to->GetValue()) m_dp_to->SetValue(m_dp_from->GetValue());
		RefreshTabs();
	});
	m_dp_to->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
		if (m_dp_to->GetValue() < m_dp_from->GetValue()) m_dp_from->SetValue(m_dp_to->GetValue());
		RefreshTabs();
	});

	// Record タブで record を編集したら、Detail の表示も更新する
	Bind(wxEVT_MENU, [this](wxCommandEvent&) {
		wxCommandEvent evt;
		OnSetTextOfDetail(evt);
		RefreshTabs();
	}, ID_UPDATE_STATISTIC);


    pnlDetail->SetSizer(rightBoxSizer);
    this->SetSizer(mainSizer);
	CenterOnParent(); // 親ウィンドウの真ん中に表示する

    SetBackgroundColour(wxColour(189, 255, 255));
    pnlDetail->Refresh();
    
    wxCommandEvent evt;
    OnSetTitle(evt);
    OnSetTextOfDetail(evt);
    RefreshTabs();
    pnlDetail->Refresh();
}

void Detail::RefreshTabs() {
    const bool filtering = m_cb_filter->GetValue();

    // Record タブ: 絞らないときは全期間。絞るときは [From の 0 時, To の翌日 0 時)
    wxDateTime start(1, wxDateTime::Jan, 1970);
    wxDateTime end(1, wxDateTime::Jan, 2100);
    if (filtering) {
        start = m_dp_from->GetValue();
        start.ResetTime();
        end = m_dp_to->GetValue();
        end.ResetTime();
        end.Add(wxDateSpan::Day());
    }
    m_activity_report->SetPeriodAndRefresh(start, end);
    m_record_viewer->SetPeriodAndRefresh(start, end);
    m_todo_panel->SetPeriod(filtering, m_dp_from->GetValue(), m_dp_to->GetValue());

    m_goal_tab->Load(filtering, m_dp_from->GetValue(), m_dp_to->GetValue());
}

void Detail::OnSetTitle(wxCommandEvent &event) {
    std::string title_str = m_db.GetCategoryName(m_id);
    m_title->SetLabel(wxString::FromUTF8(title_str));

    wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    m_title->SetFont(font);
}

void Detail::OnSetTextOfDetail(wxCommandEvent &event) {
	// 全期間の合計時間取得用（1970〜現在で固定）
	std::string start_utc_all = "1970-01-01 00:00:00";
	std::string end_utc_all = wxDateTime::Now().ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();

	// DBへ
	long long total_sec_all = m_db.GetTotalTime(m_id, start_utc_all, end_utc_all);
	long long total_sec_incl_subitems = Utils::GetTimeOfChildCategories(m_db, m_id, start_utc_all, end_utc_all);
	long long last_executed = m_db.GetLastExecuted(m_id);

	// 表示
	m_result_total_time_incl_subitems->SetLabel(TimeUtils::FormatSeconds(total_sec_incl_subitems));
	m_result_total_time_selected->SetLabel(TimeUtils::FormatSeconds(total_sec_all));
	m_last_executed->SetLabel(TimeUtils::FormatEpochToDate(last_executed));

	// Goal の要約。この Category に設定された goal を、今の時点で評価して件数を数える
	const long long now = static_cast<long long>(std::time(nullptr));
	int achieved = 0, unachieved = 0, invalid = 0;
	const std::vector<Database::Goal> goals = m_db.GetGoalsByTarget(m_id, -1);
	for (const Database::Goal& g : goals) {
		const GoalEval::State state = GoalStatus::EvaluateNow(m_db, g, now).result.state;
		switch (GoalStatus::GroupOf(state)) {
		case GoalStatus::Group::Achieved:   ++achieved;   break;
		case GoalStatus::Group::Unachieved: ++unachieved; break;
		case GoalStatus::Group::Invalid:    ++invalid;    break;
		}
	}
	m_goal_total->SetLabel(wxString::Format("%d", (int)goals.size()));
	m_goal_achieved->SetLabel(wxString::Format("%d", achieved));
	m_goal_unachieved->SetLabel(wxString::Format("%d", unachieved));
	m_goal_invalid->SetLabel(wxString::Format("%d", invalid));
}
