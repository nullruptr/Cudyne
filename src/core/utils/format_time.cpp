#include "format_time.hpp"
#include <ctime>

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
}
