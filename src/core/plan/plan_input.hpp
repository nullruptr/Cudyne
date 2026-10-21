#pragma once
// Add Rows 画面の、年・月・日・時・分・秒の入力欄の検証。仕様は plan.md。DB と wxWidgets には依存しない
#include <string>

namespace PlanInput {

struct Date {
    int year = 0;
    int month = 0;
    int day = 0;
};

struct TimeRange {
    int start_seconds = 0;  // 開始の日の 0 時からの秒
    int end_seconds = 0;    // 同じく。終了が翌日以降なら 86400 以上
};

constexpr int kMaxStartHour = 23;   // 開始は 0〜23 時
constexpr int kMaxEndHour = 71;     // 終了は、開始の日から 2 日後の 23 時まで (24 以上 = 翌日以降)

namespace detail {

inline bool IsLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

inline int DaysInMonth(int y, int m) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && IsLeapYear(y)) ? 29 : days[m - 1];
}

} // namespace detail

// 入力欄の文字列 (前後の空白は無視) を、0 以上の整数にする。数字だけで、max_digits 桁以内のときだけ true。空は false
inline bool ParseNumber(const std::string& text, int max_digits, int& out) {
    std::string::size_type begin = text.find_first_not_of(" \t");
    if (begin == std::string::npos) return false;
    const std::string::size_type end = text.find_last_not_of(" \t");
    const std::string s = text.substr(begin, end - begin + 1);

    if (s.empty() || static_cast<int>(s.size()) > max_digits) return false;
    int value = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

// "mmdd" (ちょうど 4 桁。例: 1021) を月と日にする。実在するかどうかは MakeDate で確かめる
inline bool ParseMonthDay(const std::string& text, int& month, int& day) {
    int value = 0;
    std::string trimmed = text;
    const std::string::size_type begin = trimmed.find_first_not_of(" \t");
    if (begin == std::string::npos) return false;
    trimmed = trimmed.substr(begin, trimmed.find_last_not_of(" \t") - begin + 1);
    if (trimmed.size() != 4 || !ParseNumber(trimmed, 4, value)) return false;
    month = value / 100;
    day = value % 100;
    return true;
}

// "hhmm" を時と分にする。4 桁 (0930, 2600) か、3 桁 (930 = 9 時 30 分)、1〜2 桁 (9, 13 = 時だけ)。範囲は MakeTimeRange で確かめる
inline bool ParseHourMinute(const std::string& text, int& hour, int& minute) {
    int value = 0;
    if (!ParseNumber(text, 4, value)) return false;
    std::string trimmed = text;
    const std::string::size_type begin = trimmed.find_first_not_of(" \t");
    trimmed = trimmed.substr(begin, trimmed.find_last_not_of(" \t") - begin + 1);
    if (trimmed.size() <= 2) {
        hour = value;
        minute = 0;
    } else {
        hour = value / 100;
        minute = value % 100;
    }
    return true;
}

// 実在する日付か
inline bool MakeDate(int year, int month, int day, Date& out) {
    if (year < 1970 || year > 2200 || month < 1 || month > 12) return false;
    if (day < 1 || day > detail::DaysInMonth(year, month)) return false;
    out = {year, month, day};
    return true;
}

// 時・分・秒を、0 時からの秒にする。時は max_hour まで。不正なら -1
inline int ClockSeconds(int hour, int minute, int second, int max_hour) {
    if (hour < 0 || hour > max_hour || minute < 0 || minute > 59 || second < 0 || second > 59) return -1;
    return hour * 3600 + minute * 60 + second;
}

// 開始 (時・分・秒) と終了 (時・分・秒) から時間帯を作る。終了は開始より後であること。
// 日をまたぐときは、終了の時を 24 以降にする (26 時 = 翌日の 2 時)。不正なら false と、理由
inline bool MakeTimeRange(int start_hour, int start_minute, int start_second,
                          int end_hour, int end_minute, int end_second,
                          TimeRange& out, std::string& error) {
    const int start = ClockSeconds(start_hour, start_minute, start_second, kMaxStartHour);
    if (start < 0) {
        error = "Invalid start time (hour 0-23, minute and second 0-59)";
        return false;
    }
    const int end = ClockSeconds(end_hour, end_minute, end_second, kMaxEndHour);
    if (end < 0) {
        error = "Invalid end time (hour 0-71, minute and second 0-59)";
        return false;
    }
    if (end <= start) {
        error = "End must be after start";
        return false;
    }
    out.start_seconds = start;
    out.end_seconds = end;
    return true;
}

} // namespace PlanInput
