#include "database.hpp"

// ToDo ID に紐付く record の合計時刻取得 (期間と重なる部分のみ)
long long Database::GetTotalTimeByTodo(int todo_id, const std::string& start_utc, const std::string& end_utc) {
	if (db == nullptr) return 0;

	const char* sql =
	"SELECT SUM("
	"  strftime('%s', MIN(time_end, ?)) - "
	"  strftime('%s', MAX(time_begin, ?))"
	") "
	"FROM records "
	"WHERE todo_id = ? "
	"AND time_end > ? "
	"AND time_begin < ?;";

	sqlite3_stmt* stmt;
	long long total_seconds = 0;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, end_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, start_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 3, todo_id);
		sqlite3_bind_text(stmt, 4, start_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 5, end_utc.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			total_seconds = sqlite3_column_int64(stmt, 0);
		}
	}
	sqlite3_finalize(stmt);
	return total_seconds;
}

// IDの合計時刻取得
long long Database::GetTotalTime(int category_id, const std::string& start_utc, const std::string& end_utc) {
	if (db == nullptr) return 0;

	// STRFTIME の%s で、1970-01-01 00:00:00 UTCからの秒数を示す。
	const char* sql =
	"SELECT SUM("
	"  strftime('%s', MIN(time_end, ?)) - "
	"  strftime('%s', MAX(time_begin, ?))"
	") "
	"FROM records "
	"WHERE category_id = ? "
	"AND time_end > ? "
	"AND time_begin < ?;";

	sqlite3_stmt* stmt;
	long long total_seconds = 0;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, end_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, start_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 3, category_id);
		sqlite3_bind_text(stmt, 4, start_utc.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 5, end_utc.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			total_seconds = sqlite3_column_int64(stmt, 0);
		}
	}
	sqlite3_finalize(stmt);
	return total_seconds;
}
