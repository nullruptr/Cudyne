#include "controller.hpp"
#include "core/utils/format_time.hpp"
#include "gui/mainwnd/mainwnd.hpp"
#include <string>
#include <wx/datetime.h>
#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/wx.h>


Controller::Controller(wxWindow* parent, Database &dbRef)
	: wxPanel(parent, wxID_ANY)
	  , m_db(dbRef) {
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// --- レンジ ---
	wxBoxSizer* range_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* range_display = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* range_update = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* date_label = new wxStaticText(this, wxID_ANY, _("Range: "));
	m_date_range = new wxChoice(this, wxID_ANY);

	m_date_range->Append(_("Today"));
	m_date_range->Append(_("This Week"));
	m_date_range->Append(_("This Month"));
	m_date_range->Append(_("This Year"));
	m_date_range->Append(_("Last 1 Month"));
	m_date_range->Append(_("Last 3 Months"));
	m_date_range->Append(_("Last 6 Months"));
	m_date_range->Append(_("Last 1 Year"));
	m_date_range->Append(_("Last 3 Years"));
	m_date_range->Append(_("Custom"));

	// 最初の項目（Today）をデフォルトで選択状態にする
	if (!m_date_range->IsEmpty()) {
		m_date_range->SetSelection(0);
	}

	m_period_display = new wxStaticText(this, wxID_ANY, wxEmptyString);
	m_date_picker_start = new wxDatePickerCtrl(this, wxID_ANY);
	m_date_picker_end = new wxDatePickerCtrl(this, wxID_ANY);

	// 初期状態ではDatePickerを隠す
	m_date_picker_start->Hide();
	m_date_picker_end->Hide();

	m_btn_update = new wxButton(this, wxID_ANY, _("Update"));
	m_cb_auto_update = new wxCheckBox(this, wxID_ANY, _("Auto Update"));
	// デフォルトでチェックあり
	m_cb_auto_update->SetValue(true);

	range_sizer->Add(date_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
	range_sizer->Add(m_date_range, 0, wxALIGN_CENTER_VERTICAL);
	range_display->Add(m_period_display, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
	range_display->Add(m_date_picker_start, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
	range_display->Add(m_date_picker_end, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
	range_update->Add(m_btn_update, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
	range_update->Add(m_cb_auto_update, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(5));
	wxBoxSizer* range_group = new wxBoxSizer(wxVERTICAL);
	range_group->Add(range_sizer,   0, wxBOTTOM, FromDIP(7));
	range_group->Add(range_display, 0, wxBOTTOM, FromDIP(7));
	range_group->Add(range_update,  0, 0, 0);
	sizer->Add(range_group, 0, wxALL, FromDIP(10));

		// --- オフセット操作エリア ---
	// 3行（年・月・日）× 3列（ラベル・前へ・次へ）のグリッド
	wxFlexGridSizer* offset_grid = new wxFlexGridSizer(3, 3, 5, 5);

	// ボタンのサイズ規定（小さく設定）
	wxSize btn_size(FromDIP(33), FromDIP(20));

	// --- 年 ---
	wxStaticText* label_y = new wxStaticText(this, wxID_ANY, _("Year: "));
	m_btn_offset_y_prev = new wxButton(this, wxID_ANY, "<", wxDefaultPosition, btn_size);
	m_btn_offset_y_next = new wxButton(this, wxID_ANY, ">", wxDefaultPosition, btn_size);

	// --- 月 ---
	wxStaticText* label_m = new wxStaticText(this, wxID_ANY, _("Month: "));
	m_btn_offset_m_prev = new wxButton(this, wxID_ANY, "<", wxDefaultPosition, btn_size);
	m_btn_offset_m_next = new wxButton(this, wxID_ANY, ">", wxDefaultPosition, btn_size);

	// --- 日 ---
	wxStaticText* label_d = new wxStaticText(this, wxID_ANY, _("Day: "));
	m_btn_offset_d_prev = new wxButton(this, wxID_ANY, "<", wxDefaultPosition, btn_size);
	m_btn_offset_d_next = new wxButton(this, wxID_ANY, ">", wxDefaultPosition, btn_size);

	// グリッドに追加（順番に埋まっていく）
	offset_grid->Add(label_y, 0, wxALIGN_CENTER_VERTICAL);
	offset_grid->Add(m_btn_offset_y_prev);
	offset_grid->Add(m_btn_offset_y_next);

	offset_grid->Add(label_m, 0, wxALIGN_CENTER_VERTICAL);
	offset_grid->Add(m_btn_offset_m_prev);
	offset_grid->Add(m_btn_offset_m_next);

	offset_grid->Add(label_d, 0, wxALIGN_CENTER_VERTICAL);
	offset_grid->Add(m_btn_offset_d_prev);
	offset_grid->Add(m_btn_offset_d_next);

	sizer->Add(offset_grid, 0, wxALL, FromDIP(10));

	this->SetSizer(sizer);

	m_date_range->Bind(wxEVT_CHOICE, &Controller::OnRangeChanged, this);
	// controller.cpp のコンストラクタ内
	// event 引数が2つ以上あるので、ラムダ式で対応
	m_btn_update->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
		this->OnUpdateStatistics(event, EventType::FROM_MYSELF);
	});

	m_date_picker_start->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent& event) {
		this->OnUpdateStatistics(event, EventType::FROM_MYSELF);
	});

	m_date_picker_end->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent& event) {
		this->OnUpdateStatistics(event, EventType::FROM_MYSELF);
	});

	// 日付遷移ボタンイベント
	m_btn_offset_y_prev->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::Y_PREV);
	});
	m_btn_offset_y_prev->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::Y_PREV);
	});
	m_btn_offset_y_next->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::Y_NEXT);
	});
	m_btn_offset_m_prev->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::M_PREV);
	});
	m_btn_offset_m_next->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::M_NEXT);
	});
	m_btn_offset_d_prev->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::D_PREV);
	});
	m_btn_offset_d_next->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
	    this->OnOffsetDate(event, OffsetDateButton::D_NEXT);
	});

	// 初回起動時に期間を表示させるためのダミーイベント
	wxCommandEvent dummy;
	OnRangeChanged(dummy);
  }

void Controller::OnRangeChanged(wxCommandEvent& event) {
	int selIdx = m_date_range->GetSelection();
	if (selIdx == wxNOT_FOUND) return;

	wxDateTime now = wxDateTime::Now();
	wxDateTime start = now, end = now;

	// 時刻を00:00:00にリセットして日付ベースの計算に
	start.ResetTime();
	// end は「翌日の00:00:00」
	end = start + wxDateSpan(0, 0, 0, 1); // +1日

	switch (selIdx) {
		case RANGE_TODAY:
			break;

		case RANGE_THIS_WEEK:
			// 月曜日開始
			start = now.GetWeekDayInSameWeek(wxDateTime::Mon);
			start.ResetTime();
			break;

		case RANGE_THIS_MONTH:
			start.SetDay(1);
			start.ResetTime();
			break;

		case RANGE_THIS_YEAR:
			start.SetMonth(wxDateTime::Jan);
			start.SetDay(1);
			start.ResetTime();
			break;

		case RANGE_LAST_1_MONTH:
			start.Subtract(wxDateSpan(0, 1, 0, 0));
			break;

		case RANGE_LAST_3_MONTHS:
			start.Subtract(wxDateSpan(0, 3, 0, 0));
			break;

		case RANGE_LAST_6_MONTHS:
			start.Subtract(wxDateSpan(0, 6, 0, 0));
			break;

		case RANGE_LAST_1_YEAR:
			start.Subtract(wxDateSpan(1, 0, 0, 0));
			break;

		case RANGE_LAST_3_YEARS:
			start.Subtract(wxDateSpan(3, 0, 0, 0));
			break;

		case RANGE_CUSTOM:
			// Custom の場合は表示を切り替えて即リターン
			m_period_display->Hide();
			m_date_picker_start->Show();
			m_date_picker_end->Show();
			this->GetSizer()->Layout();
			// 安全のため初期化
			start.ResetTime();
			end.ResetTime();
			end = start + wxDateSpan(0, 0, 0, 1);
			break;

			default:
			return;
	}


	// 時刻情報をメンバ変数に渡す。Update でこれを利用する。
	m_current_start = start;
	m_current_end = end;


	// custom のとき
	if (selIdx == RANGE_CUSTOM) {
		start = m_date_picker_start->GetValue();
		end = m_date_picker_end->GetValue();

		start.ResetTime();
		end.ResetTime();
		end += wxDateSpan(0, 0, 0, 1);
	} else {
		// Custom 以外の場合の共通処理
		m_date_picker_start->Hide();
		m_date_picker_end->Hide();
		m_period_display->Show();
	}


	UpdatePeriodDisplay();

	// m_cb_auto_update が true のとき
	if (m_cb_auto_update->GetValue()) {
		wxCommandEvent evt_update;
		OnUpdateStatistics(evt_update, EventType::FROM_MYSELF);
	}
}

void Controller::UpdatePeriodDisplay() {
	m_period_display->SetLabel(
		m_current_start.FormatISODate() + " - " + m_current_end.FormatISODate()
	);
	this->GetSizer()->Layout();
}



void Controller::OnOffsetDate(wxCommandEvent& event, OffsetDateButton btn) {
    switch (btn) {
	case OffsetDateButton::Y_PREV:
	case OffsetDateButton::Y_NEXT: {
	    int dir = (btn == OffsetDateButton::Y_PREV) ? -1 : 1;
	    if ( // 範囲が同じ年でかつ開始点が01/01、終了点が12/31 の時
		m_current_start.GetYear() == m_current_end.GetYear() &&
		m_current_start.GetMonth() == wxDateTime::Jan &&
		m_current_start.GetDay() == 1 &&
		m_current_end.GetMonth() == wxDateTime::Dec &&
		m_current_end.GetDay() == 31
	    ) {
		// そのまま1年ずらす
		m_current_start.Add(wxDateSpan(dir, 0, 0, 0));
		m_current_end.Add(wxDateSpan(dir, 0, 0, 0));
	    } else {
		// それ以外の時、年を揃えて01/01〜12/31 にする
		if (btn == OffsetDateButton::Y_PREV) {
		    m_current_end.SetYear(m_current_start.GetYear());
		} else {
		    m_current_start.SetYear(m_current_end.GetYear());
		}
		m_current_start.SetMonth(wxDateTime::Jan);
		m_current_start.SetDay(1);
		m_current_end.SetMonth(wxDateTime::Dec);
		m_current_end.SetDay(31);
	    }
	    break;
	}
	case OffsetDateButton::M_PREV:
	case OffsetDateButton::M_NEXT: {
	    int dir = (btn == OffsetDateButton::M_PREV) ? -1 : 1;
	    if ( // 範囲が同じ月で、月初めと月末のとき
		m_current_start.GetYear() == m_current_end.GetYear() &&
		m_current_start.GetMonth() == m_current_end.GetMonth() &&
		m_current_start.GetDay() == 1 &&
		m_current_end.GetDay() == TimeUtils::LastDayOfMonth(m_current_end.GetYear(), m_current_end.GetMonth())
	    ) {
		// 1ヶ月ずらして、新しい月の月初〜月末にする
		m_current_start.Add(wxDateSpan(0, dir, 0, 0));
		m_current_start.SetDay(1);
		m_current_end = m_current_start;
		m_current_end.SetDay(TimeUtils::LastDayOfMonth(
		    m_current_start.GetYear(), m_current_start.GetMonth()));
	    } else {
		// それ以外の時、月を揃えて月初〜月末にする
		if (btn == OffsetDateButton::M_PREV) {
		    m_current_end.SetMonth(m_current_start.GetMonth());
		    m_current_end.SetYear(m_current_start.GetYear());
		} else {
		    m_current_start.SetMonth(m_current_end.GetMonth());
		    m_current_start.SetYear(m_current_end.GetYear());
		}
		m_current_start.SetDay(1);
		m_current_end.SetDay(TimeUtils::LastDayOfMonth(m_current_end.GetYear(), m_current_end.GetMonth()));
	    }
	    break;
	}
	case OffsetDateButton::D_PREV:
	case OffsetDateButton::D_NEXT: {
	    int dir = (btn == OffsetDateButton::D_PREV) ? -1 : 1;
	    m_current_start.Add(wxDateSpan(0, 0, 0, dir));
	    m_current_end.Add(wxDateSpan(0, 0, 0, dir));
	    break;
	}
    }
    UpdatePeriodDisplay();
    // 更新イベントを発火
    wxCommandEvent evt;
    OnUpdateStatistics(evt, EventType::FROM_MYSELF);
}

void Controller::OnUpdateStatistics(wxCommandEvent& event, EventType type) {

	// RANGE_CUSTOM の場合、ボタンを押した瞬間の Picker の値を再取得する
	if (m_date_range->GetSelection() == RANGE_CUSTOM) {
		m_current_start = m_date_picker_start->GetValue();
		m_current_end = m_date_picker_end->GetValue();
		m_current_start.ResetTime();
		m_current_end += wxDateSpan(0, 0, 1, 0);
	}

	// 指定された時刻をDBに渡せるようstd::string へ
	std::string start_utc = m_current_start.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
	std::string end_utc = m_current_end.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
	// 全範囲の合計時間取得用
	std::string start_utc_all = "1970-01-01 00:00:00";
	std::string end_utc_all   = wxDateTime::Now().ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();

	if (m_selected_id != -1)
	{
	    // DBへ
	    long long total_sec = m_db.GetTotalTime(m_selected_id, start_utc, end_utc);
	    long long total_sec_all = m_db.GetTotalTime(m_selected_id, start_utc_all, end_utc_all);
	};

	// 表示内容更新イベント
	// 無限ループ防止のために、MAINWND から更新処理が来たら、除外する
	if (type == EventType::FROM_MYSELF) {
		// activity report 更新イベントを投げる
		wxCommandEvent evt_activity_report(wxEVT_MENU, ID_ACTIVITY_REPORT);
		evt_activity_report.SetEventObject(this);
		wxPostEvent(GetParent(), evt_activity_report);

		// statistic に 更新イベントを投げる
		wxCommandEvent evt_statistic(wxEVT_MENU, ID_UPDATE_STATISTIC);
		evt_statistic.SetEventObject(this);
		wxPostEvent(GetParent(), evt_statistic);
	}
}

// MainWnd から期間を取得するためのゲッタ
void Controller::GetCurrentRange(wxDateTime& start, wxDateTime& end) const {
	start = m_current_start;
	end = m_current_end;
}
