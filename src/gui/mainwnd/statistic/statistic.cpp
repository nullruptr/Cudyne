#include "gui/mainwnd/statistic/statistic.hpp"
#include "core/utils/format_time.hpp"
#include <wx/event.h>
#include <wx/filefn.h>

Statistic::Statistic(wxWindow* parent, Database &dbRef) 
	: wxPanel(parent, wxID_ANY)
	, m_db(dbRef) {
		
	
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer* stat_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Statistics"));
	wxFlexGridSizer* stat_grid = new wxFlexGridSizer(4, 2, 8, 20);

	wxStaticText* stat_total_time_all = new wxStaticText(this, wxID_ANY, _("Total Time (All-time):"));
	m_result_total_time_all = new wxStaticText(this, wxID_ANY, _("00:00:00"));
	wxStaticText* stat_total_time_range = new wxStaticText(this, wxID_ANY, _("Total Time (Selected Range):"));
	m_result_total_time_range = new wxStaticText(this, wxID_ANY, _("00:00:00"));
	wxStaticText* stat_streak = new wxStaticText(this, wxID_ANY, _("Current Streak:"));
	wxStaticText* result_streak = new wxStaticText(this, wxID_ANY, _("0d"));
	wxStaticText* stat_last_executed = new wxStaticText(this, wxID_ANY, _("Last Executed:"));
	m_last_executed = new wxStaticText(this, wxID_ANY, ("--"));

	stat_grid->Add(stat_total_time_all, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(m_result_total_time_all, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(stat_total_time_range, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(m_result_total_time_range, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(stat_streak, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(result_streak, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(stat_last_executed, 0, wxALIGN_CENTER_VERTICAL);
	stat_grid->Add(m_last_executed, 0, wxALIGN_CENTER_VERTICAL);

	stat_box->Add(stat_grid, 1, wxALL | wxEXPAND, 10);
	sizer->Add(stat_box, 1, wxALL | wxEXPAND, 5); // ボックスをメインサイザーへ

	this->SetSizer(sizer); // パネルにサイザーをセット
	sizer->Fit(this);      // パネルの最小サイズを計算
}

void Statistic::OnUpdateStatistic() {

	// 指定された時刻をDBに渡せるようstd::string へ
	std::string start_utc = m_current_start.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
	std::string end_utc = m_current_end.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
	// 全範囲の合計時間取得用
	std::string start_utc_all = "1970-01-01 00:00:00";

	// DBへ
	long long total_sec = m_db.GetTotalTime(m_selected_id, start_utc, end_utc);
	long long total_sec_all = m_db.GetTotalTime(m_selected_id, start_utc_all, end_utc);
	long long last_executed = m_db.GetLastExecuted(m_selected_id);

	// 表示
	// 分と秒は2桁固定
	m_result_total_time_range->SetLabel(TimeUtils::FormatSeconds(total_sec));
	m_result_total_time_all->SetLabel(TimeUtils::FormatSeconds(total_sec_all));
	m_last_executed->SetLabel(TimeUtils::FormatEpochToDate(last_executed));
}

// Mainwnd から、先にID の見送られてくるので、ID を受け取る
// Mainwnd 側から、更新処理が呼ばれるのでそのあとにメンバ変数の時刻を取得
void Statistic::GetId(int id) {
	m_selected_id = id;
}

void Statistic::GetCurrentRange(const wxDateTime& start, const wxDateTime& end) {
	m_current_start = start;
	m_current_end = end;
	OnUpdateStatistic();
}
