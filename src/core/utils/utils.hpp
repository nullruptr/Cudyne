#pragma once
#include <string>
#include <wx/wx.h>
#include "core/db/database.hpp"

namespace Utils {
    // 与えられたid自身とそれ以下の時間の合計値(秒)を取得する。期間は UTC 文字列("%Y-%m-%d %H:%M:%S")で指定する。
    long long GetTimeOfChildCategories(Database& db, int id,
                                       const std::string& start_utc,
                                       const std::string& end_utc);
}
