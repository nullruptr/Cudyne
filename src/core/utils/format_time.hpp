#pragma once
#include <wx/string.h>

namespace TimeUtils {
    // 秒を "HH:MM:SS" 形式に変換する
    wxString FormatSeconds(long long total_seconds);
    // epoch 秒を 2026-06-01 20:39:12 のwxStringにする
    wxString FormatEpochToDate(long long epoch);
}
