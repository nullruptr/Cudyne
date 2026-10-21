#include "activity_report.hpp"
#include "core/utils/format_time.hpp"
#include <wx/listbase.h>
#include <wx/wx.h>
#include <wx/dateevt.h>
#include <algorithm>
#include <vector>

ActivityReport::ActivityReport(wxWindow* parent, Database& dbRef)
    : wxPanel(parent, wxID_ANY)
    , m_db(dbRef)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // --- 中部: リスト ---
    m_list = new wxListCtrl(this, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
    m_list->InsertColumn(0, _("ID"), wxLIST_FORMAT_LEFT, FromDIP(33));
    m_list->InsertColumn(1, _("Path"), wxLIST_FORMAT_LEFT, FromDIP(133));
    m_list->InsertColumn(2, _("Category"), wxLIST_FORMAT_LEFT, FromDIP(80));
    m_list->InsertColumn(3, _("Total Time"), wxLIST_FORMAT_LEFT, FromDIP(66));
    m_list->InsertColumn(4, _("Ratio (%)"), wxLIST_FORMAT_RIGHT, FromDIP(50));
    sizer->Add(m_list, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(6));

    // --- 下部: 合計表示 ---
    m_total_label = new wxStaticText(this, wxID_ANY, _("Total: 00:00:00"));
    sizer->Add(m_total_label, 0, wxALL, 10);

    this->SetSizer(sizer);

    CenterOnParent();
}


void ActivityReport::LoadReport() {
	m_list->DeleteAllItems();
	if (!m_current_start.IsValid()) return; //安全のため

	std::string s_utc = m_current_start.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
	std::string e_utc = m_current_end.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();

	std::vector<Database::RecordSummary> summaries = m_db.GetRecordsByRange(s_utc, e_utc);

	// カテゴリが指定されていたら、そのカテゴリ以下(自分自身と配下の項目)の集計だけを残す
	if (m_category_id != -1) {
		std::vector<int> ids{m_category_id};
		std::vector<int> children;
		if (m_db.GetChildCategories(children, m_category_id)) ids.insert(ids.end(), children.begin(), children.end());
		summaries.erase(std::remove_if(summaries.begin(), summaries.end(), [&](const Database::RecordSummary& rs) {
			return std::find(ids.begin(), ids.end(), rs.category_id) == ids.end();
		}), summaries.end());
	}

	// ず最初に秒数を計算
	long long total_seconds = 0;
	for (const auto& rs : summaries) {
		total_seconds += rs.total_seconds;
	}

	for (const auto& rs : summaries) {
		// ID
		long idx = m_list->InsertItem(m_list->GetItemCount(), wxString::Format("%d", rs.category_id));

		// Path
		std::string path = m_db.GetCategoriesPath(rs.category_id);
		m_list->SetItem(idx, 1, wxString::FromUTF8(path));
		// カテゴリ名
		m_list->SetItem(idx, 2, wxString::FromUTF8(rs.category_name));
		// 合計時間
		int h = rs.total_seconds / 3600;
		int m = (rs.total_seconds % 3600) / 60;
		int s = rs.total_seconds % 60;
        
		m_list->SetItem(idx, 3, TimeUtils::FormatSeconds(rs.total_seconds));

		// 割合
		double ratio = 0.0;
		if (total_seconds > 0) {
			// (取得秒数 / 合計秒数) * 100 => 割合
			ratio = (static_cast<double>(rs.total_seconds) / total_seconds) * 100.0;
		}
		m_list->SetItem(idx, 4, wxString::Format("%.1f%%", ratio));
	}

	m_total_label->SetLabel(_("Total: ") + TimeUtils::FormatSeconds(total_seconds));
}

void ActivityReport::SetCategoryId(int category_id) {
	m_category_id = category_id;
}

void ActivityReport::SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end) {
	m_current_start = start;
	m_current_end = end;

	// 再計算と表示更新
	LoadReport();
}
