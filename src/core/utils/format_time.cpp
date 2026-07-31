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

    wxString ParseHHMM(const wxString& input) {
        if (input.Find(':') != wxNOT_FOUND) {
            wxString hh_str = input.BeforeFirst(':');
            wxString mm_str = input.AfterFirst(':');
            long hh = 0, mm = 0;
            if (!hh_str.ToLong(&hh) || !mm_str.ToLong(&mm)) return "";
            if (hh < 0 || hh > 23) return "";
            if (mm < 0 || mm > 59) return "";
            return wxString::Format("%02d:%02d", hh, mm);
        }
        long val = 0;
        if (!input.ToLong(&val)) return "";
        int h = val / 100;
        int m = val % 100;
        if (h < 0 || h > 23) return "";
        if (m < 0 || m > 59) return "";
        return wxString::Format("%02d:%02d", h, m);
    }

    wxString ParseSS(const wxString& input) {
        long val = 0;
        if (!input.ToLong(&val)) return "";
        if (val < 0 || val > 59) return "";
        return wxString::Format("%02d", val);
    }

    std::string BuildUTCString(const wxDateTime& date, const wxString& hhmm, const wxString& ss) {
    long hh = 0, mm = 0, sec = 0;
    hhmm.BeforeFirst(':').ToLong(&hh);
    hhmm.AfterFirst(':').ToLong(&mm);
    ss.ToLong(&sec);

    wxDateTime dt = date;
    dt.SetHour(hh);
    dt.SetMinute(mm);
    dt.SetSecond(sec);

    return dt.ToUTC().Format("%Y-%m-%d %H:%M:%S").ToStdString();
}

    ParsedTime ParseUTCString(const std::string& utc_str) {
        ParsedTime pt;
        if (utc_str.empty()) {
            pt.date = wxDateTime::Today();
            pt.hhmm = "00:00";
            pt.ss   = "00";
            return pt;
        }

        wxDateTime dt;
        dt.ParseFormat(wxString::FromUTF8(utc_str), "%Y-%m-%d %H:%M:%S");
        dt = dt.FromUTC(); // 保存時は ToUTC() しているので、ここで逆変換してシステム時間に戻す

        pt.date = dt;
        pt.hhmm = dt.Format("%H:%M");
        pt.ss   = dt.Format("%S");
        return pt;
    }

    wxString FormatUTCStringToLocal(const std::string& utc_str) {
        if (utc_str.empty()) return wxString("-");

        wxDateTime dt;
        dt.ParseFormat(wxString::FromUTF8(utc_str), "%Y-%m-%d %H:%M:%S");
        dt = dt.FromUTC();

        return dt.Format("%Y-%m-%d %H:%M:%S");
    }

    long long ParseUTCStringToEpoch(const std::string& utc_str) {
        if (utc_str.empty()) return 0;

        wxDateTime dt;
        dt.ParseFormat(wxString::FromUTF8(utc_str), "%Y-%m-%d %H:%M:%S");
        dt = dt.FromUTC();

        return (long long)dt.GetTicks();
    }

    wxString FormatCountdown(long long target_epoch, long long now_epoch) {
        long long diff = target_epoch - now_epoch;
        bool overdue = diff < 0;
        long long abs_diff = overdue ? -diff : diff;

        long long days = abs_diff / 86400;
        long long rem  = abs_diff % 86400;
        long long h = rem / 3600;
        long long m = (rem % 3600) / 60;
        long long s = rem % 60;

        return wxString::Format("%s%lldd %02lld:%02lld:%02lld", overdue ? "-" : "", days, h, m, s);
    }
}
