#include <ctime>
#include <wx/datetime.h>
#include "format_time.hpp"


namespace TimeUtils {
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

    int CalcDurationSeconds(const std::string& begin, const std::string& end) {
        struct tm tb = {}, te = {};
        sscanf(begin.c_str(), "%d-%d-%d %d:%d:%d",
               &tb.tm_year, &tb.tm_mon, &tb.tm_mday,
               &tb.tm_hour, &tb.tm_min, &tb.tm_sec);
        sscanf(end.c_str(), "%d-%d-%d %d:%d:%d",
               &te.tm_year, &te.tm_mon, &te.tm_mday,
               &te.tm_hour, &te.tm_min, &te.tm_sec);
        tb.tm_year -= 1900; tb.tm_mon -= 1;
        te.tm_year -= 1900; te.tm_mon -= 1;
        return (int)(mktime(&te) - mktime(&tb));
    }

    int LastDayOfMonth(int year, wxDateTime::Month month) {
	wxDateTime dt(1, month, year);

	dt += wxDateSpan::Month();
	dt -= wxDateSpan::Day();

	return dt.GetDay();
    }
}
