// DB のレコード処理を担当

#include "core/db/database.hpp"
#include <iostream>
#include <sqlite3.h>

// レコード開始時刻を insert し、最後に追加したレコードをreturn
long long Database::StartRecord(int category_id, int todo_id) {
	if (db == nullptr){
		return -1;
	}

	const char* sql = "INSERT INTO records (category_id, todo_id, time_begin, time_end) "
		      "VALUES (?, ?, datetime('now'), '') RETURNING id;";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
		std::cerr << "Prepare Error: " << sqlite3_errmsg(db) << std::endl;
		return -1;
	}


	sqlite3_bind_int(stmt, 1, category_id);
	if (todo_id > 0) {
		sqlite3_bind_int(stmt, 2, todo_id);
	} else {
		sqlite3_bind_null(stmt, 2);
	}

	// 実行
	int rc = sqlite3_step(stmt);

	long long returning_id = -1;
	if (rc == SQLITE_ROW) { // 書き込み先idを格納
		returning_id = sqlite3_column_int64(stmt, 0);
	} else {
		std::cerr << "Execution Error: " << sqlite3_errmsg(db) << std::endl;
	}
	sqlite3_finalize(stmt); // stmt 解法
	return returning_id;
}


bool Database::EndRecord(int record_id) {
	if (db == nullptr){ 
		return false;
	}

	const char* sql = "UPDATE records SET time_end = datetime('now') WHERE id = ?;";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
		std::cerr << "Prepare Error: " << sqlite3_errmsg(db) << std::endl; 
		return false;
	}


	sqlite3_bind_int64(stmt, 1, record_id);

	// 実行
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) { 
		std::cerr << "Execution Error: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_finalize(stmt); // stmt 解法
	return (rc == SQLITE_DONE);
}
