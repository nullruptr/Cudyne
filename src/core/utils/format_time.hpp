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
    // "1300" -> "13:00", 不正な値は空文字を返す
    wxString ParseHHMM(const wxString& input);
    // "30" -> "30", 不正な値は空文字を返す
    wxString ParseSS(const wxString& input);
    // DatePicker の日付と hhmm, ss 文字列から UTC の std::string を作る
    std::string BuildUTCString(const wxDateTime& date, const wxString& hhmm, const wxString& ss);
    // DB に保存された UTC 文字列("%Y-%m-%d %H:%M:%S")を、システム(ローカル)時間の date, hhmm, ss に分けて返す
    // 空文字の場合は今日・00:00:00 を返す
    ParsedTime ParseUTCString(const std::string& utc_str);
    // DB に保存された UTC 文字列を、システム(ローカル)時間の "2026-06-01 20:39:12" 形式の文字列にする
    // 空文字の場合は "-" を返す
    wxString FormatUTCStringToLocal(const std::string& utc_str);
    // DB に保存された UTC 文字列を epoch(Unix time, UTC) に変換する。空文字の場合は 0 を返す
    long long ParseUTCStringToEpoch(const std::string& utc_str);
    // 2つの epoch の差を "HH:MM:SS" 形式にする。target が過去なら "-" を付ける
    wxString FormatCountdown(long long target_epoch, long long now_epoch);
}
