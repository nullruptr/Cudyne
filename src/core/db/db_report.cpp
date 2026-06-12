#include "database.hpp"

std::vector<Database::RecordSummary> Database::GetRecordsByRange(const std::string& start_utc, const std::string& end_utc) {
    std::vector<RecordSummary> results;
    if (!db) return results;

    // 1. records と categories を結合
    // 2. 指定期間(start_utc ～ end_utc)に1秒でも重なっている記録を抽出
    // 3. MIN(終了時間, 指定期間末) - MAX(開始時間, 指定期間始) で、重なっている秒数を算出
    // 4. カテゴリIDごとに SUM で合計
    const char* sql = 
        "SELECT c.id, c.name, SUM("
        "  strftime('%s', MIN(r.time_end, ?)) - "
        "  strftime('%s', MAX(r.time_begin, ?))"
        ") as duration "
        "FROM records r "
        "JOIN categories c ON r.category_id = c.id "
        "WHERE r.time_end > ? AND r.time_begin < ? "
        "GROUP BY c.id "
        "ORDER BY duration DESC;"; // 時間が長い順に並べる

	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		return results;
	}

	// パラメータのバインド (Dashboardと同じ順序・型)
	sqlite3_bind_text(stmt, 1, end_utc.c_str(), -1, SQLITE_TRANSIENT);   // MIN用
	sqlite3_bind_text(stmt, 2, start_utc.c_str(), -1, SQLITE_TRANSIENT); // MAX用
	sqlite3_bind_text(stmt, 3, start_utc.c_str(), -1, SQLITE_TRANSIENT); // WHERE用
	sqlite3_bind_text(stmt, 4, end_utc.c_str(), -1, SQLITE_TRANSIENT);   // WHERE用

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		RecordSummary rs;
		rs.category_id = sqlite3_column_int(stmt, 0);
		const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		rs.category_name = name ? name : "Unknown";
		rs.total_seconds = sqlite3_column_int64(stmt, 2);
        
		// 0秒以下のゴミデータ（ロジック上は発生しないはずだが安全策）を除外
		if (rs.total_seconds > 0) {
			results.push_back(rs);
		}
	}

	sqlite3_finalize(stmt);
	return results;
}
