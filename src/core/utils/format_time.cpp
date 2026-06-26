#include <ctime>
#include <wx/datetime.h>
#include "format_time.hpp"


namespace TimeUtils {
    ParsedTime ParseEpoch(long long epoch) {
    ParsedTime pt;
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm* tm = std::localtime(&t);

    char date_buf[11]; // "2026-01-01\0"
    char hhmm_buf[6];  // "13:00\0"
    char ss_buf[3];    // "00\0"

    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", tm);
    std::strftime(hhmm_buf, sizeof(hhmm_buf), "%H:%M",   tm);
    std::strftime(ss_buf,   sizeof(ss_buf),   "%S",       tm);

    pt.date.ParseDate(date_buf);
    pt.hhmm = wxString(hhmm_buf);
    pt.ss   = wxString(ss_buf);
    return pt;
}
    wxString FormatSeconds(long long total_seconds) {
	    long long h = total_seconds / 3600;
	    long long m = (total_seconds % 3600) / 60;
	    long long s = total_seconds % 60;
	    return wxString::Format("%02lld:%02lld:%02lld", h, m, s);
    }

    wxString FormatEpochToDate(long long epoch) {
	
	// 未実行 (レコードなしのとき 0 を返すため)
	if (epoch == 0) return wxString("--");
	
	// long long を time_t にキャスト
	std::time_t t = static_cast<std::time_t>(epoch);
	// 文字列に変換
	std::tm* tm = std::localtime(&t);
	// 19 文字 + 終端 null = 20
	char buf[20];
	// (書き込み先buf, サイズを知らせる, フォーマット, std::tm)
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm); 
	return wxString(buf);
    }

    int LastDayOfMonth(int year, wxDateTime::Month month) {
	wxDateTime dt(1, month, year);

	dt += wxDateSpan::Month();
	dt -= wxDateSpan::Day();

	return dt.GetDay();
    }
}
