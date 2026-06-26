#pragma once
#include <string>
#include <wx/datetime.h>
#include <wx/string.h>

namespace TimeUtils {
    struct ParsedTime {
	wxDateTime date;
	wxString   hhmm;
	wxString   ss;
    };
    // 時刻をdate, hhmm, ss に分けて wxString で return する
    ParsedTime ParseEpoch(long long epoch);
    // 秒を "HH:MM:SS" 形式に変換する
    wxString FormatSeconds(long long total_seconds);
    // epoch 秒を 2026-06-01 20:39:12 のwxStringにする
    wxString FormatEpochToDate(long long epoch);
    // 年と月から月末を算出する
    int LastDayOfMonth (int year, wxDateTime::Month);
}
