#include "core/db/database.hpp"
#include <iostream>
#include <soci/error.h>
#include <soci/rowset.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>

Database::Database() : db(nullptr) {
	
}

Database::~Database(){ // 終了時処理。安全のため、デストラクタでもクローズ。
    Close();
}

bool Database::Connect(const std::string& path) { // DB に接続 
    try {
	// すでに接続されているときは、閉じる
	if (sql.get_backend() != nullptr) {
	    sql.close();
	}
	// sqlite3.h 用の互換対応
	if (db != nullptr) {
            sqlite3_close_v2(db);
            db = nullptr;
        }

	sql.open(soci::sqlite3, path);

	// 同じく，互換対応
	sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr);

	// 外部キー制約を有効化
	sql << "PRAGMA foreign_keys = ON";

	m_get_path = path;
	return true;
    } catch (const soci::soci_error& e) {
	std::cerr << "Connect Eroor: " << e.what() << std::endl;
	return false;
    }
}

bool Database::Create(const std::string& path) { // DB 新規作成
	int rc = sqlite3_open_v2(
			path.c_str(), // C++ -> C の char に変換
			&db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, // 開いてみて、なかったら新規作成する
			nullptr);

	if (rc != SQLITE_OK){ // SQLite が、開かなかった場合の処理
		db = nullptr;
		return false;
	}

	return true;
}

bool Database::Initialize(){
	std::cout << "Starting Initialize..." << std::endl;

	if (db == nullptr){ // DB が開いていなかったら，抜ける
		return false;
	}

	const char* sql =
		"CREATE TABLE IF NOT EXISTS categories ("// 同名のテーブルが存在しない場合のみ新しいテーブルを作成し、既存の場合はエラーを出さずに処理をスキップ（無視）する
		"id INTEGER PRIMARY KEY AUTOINCREMENT," // 整数、主キー、連番(INSERT時に id を書く必要がなくなる)
		"parent_id INTEGER," // 親子関係を識別する
		"name TEXT NOT NULL," // カテゴリ名。TEXT->文字列型、NOT NULL -> 空欄禁止（何かしら値を入れる）
		"is_hidden INTEGER DEFAULT 0," // 表示モード
		"is_folder INTEGER DEFAULT 0" // 0: タスク，1:フォルダ
		")"
		";"

		"CREATE TABLE IF NOT EXISTS todo"
		"("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"category_id INTEGER,"
		"status INTEGER DEFAULT 0,"
		"priority INTEGER,"
		"todo_name TEXT NOT NULL,"
		"start_time TEXT,"
		"deadline TEXT,"
		"completion_date TEXT,"
		"memo TEXT"
		")"
		";"

		"CREATE TABLE IF NOT EXISTS records ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"category_id INTEGER NOT NULL," // カテゴリ番号。 categories の id と同じ番号に相当する
		"todo_id INTEGER," // TODO ID
		"time_begin TEXT NOT NULL," // 開始時刻
		"time_end TEXT NOT NULL," // 終了時刻
		"memo TEXT" // メモ
		")"
		";";

	char *errMsg = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg); // 書き込み処理

	if (rc != SQLITE_OK) {
		std::cerr << "SQL Error: " << errMsg << std::endl;
		sqlite3_free(errMsg);
		return false;
	}
		
	return true;	
}

std::string Database::GetDBPath() const { // 外部からpath 取得
    return m_get_path;
}


// --- stmt についてのメモ ---
//
// stmt は sqlite3_stmt* 型，&stmt は sqlite3_stmt** 型
// 関数側の仮引数を sqlite3_stmt** ppStmt とすると，呼び出し時に
// ppStmt == &stmt ...(1)
// となる．
// (1)に両辺に * を付け加えると，
// *ppStmt == *(&stmt)...(2)
// となる．
// ここで，&stmt は，stmt のアドレス，*(&stmt)は取得されたアドレスの中身を見るということから，
// *(&stmt) == stmt
// より，*(&stmt) は，stmt そのものとなる．
//
// 次に，temp をSQL構文を解析した結果を格納する構造体とする．
// SQLite 内部で，
// sqlite3_stmt* temp = malloc(...)
// とし，構造体を格納するメモリを確保する．
// temp には，確保されたメモリのアドレスが格納される．
// ここで，(2)より，
// *ppStmt == temp;
// とすると，stmt はSQL構文解析結果が書かれた temp のアドレスとなる．
//
// 従って，sqlite3_prepare_v2 以降の stmt は，temp のアドレスそのものである．
// 故に，stmt から temp のアドレスにアクセスすることができる．
//
// -------

bool Database::InsertCategories(int parent_id, const std::string &name, bool is_folder){
	if (db == nullptr){ // DB が開いていなかったら，抜ける
		return false;
	}

	// ? はプレースホルダ（後で値を流し込む場所）
	// categories に VALUES (?, ?) を INSERT する定義文．
	const char* sql = "INSERT INTO categories (name, parent_id, is_folder) VALUES (?, ?, ?);";

	// "stmt" というポインタ変数をスタックに確保
	sqlite3_stmt* stmt = nullptr;

	// 1. SQL文の準備
	// sqlite3_prepare_v2 の引数
	// 	1. db 接続ハンドル
	// 	2. SQL 文
	// 	3. SQL 文字列バイト数．-1で終端nullまで
	// 	4. sqlite3_stmt のアドレス
	// 	5. 基本 nullptr．複数SQL連結時のみ使用
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Prepare Error: " << sqlite3_errmsg(db) << std::endl; // sqlite3_prepare_v2 のときのエラー内容取得は，sqlite3_errmsg(db) らしい
	return false;
	}

	// 2. 値のバインド（? の部分に実際の値を割り当てる）
	// 
	// sqlite3_bind_text の引数
	// 	1. sqlite3_prepare_v2 で生成されたステートメント
	// 	2. プレースホルダ番号．スタートは１から
	// 	3. バインドする文字列
	// 	4. 文字列長．-1で終端nullまで
	// 	5. 文字列メモリの扱い方．安全のため，基本的に SQLITE_TRANSIENT(SQLite側でコピー作成)を使用
	//
	// 1番目の ? に name をセット
	sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
	// 2番目の ? に parent_id をセット
	// sqlite3_bind_int の引数
	// 	1. 2. は上記と同じ
	// 	3. バインドする整数値
	sqlite3_bind_int(stmt, 2, parent_id);
	sqlite3_bind_int(stmt, 3, is_folder);

	// 3. 実行
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) { 
		std::cerr << "Execution Error: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return false;
	}

	// 4. 後片付け
	sqlite3_finalize(stmt); // stmt 解法
	std::cout << "Category '" << name << "' inserted successfully." << std::endl;
	return true;
}

bool Database::InsertRecords(int category_id, const std::string &time_begin, const std::string &time_end, const std::string &memo){
	if (db == nullptr){ 
		return false;
	}

	const char* sql = "INSERT INTO records (category_id, time_begin, time_end, memo) VALUES (?, ?, ?, ?);";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
		std::cerr << "Prepare Error: " << sqlite3_errmsg(db) << std::endl; 
		return false;
	}


	sqlite3_bind_int(stmt, 1, category_id);
	sqlite3_bind_text(stmt, 2, time_begin.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, time_end.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, memo.c_str(), -1, SQLITE_TRANSIENT);

	// 実行
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) { 
		std::cerr << "Execution Error: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_finalize(stmt); // stmt 解法
	return true;
}

bool Database::UpdateRecords(int record_id, const std::string& time_begin, const std::string& time_end, const std::string& memo) {
    if (sql.get_backend() == nullptr) return false;

    try {
        sql << "UPDATE records SET time_begin = :begin, time_end = :end, memo = :memo WHERE id = :id",
            soci::use(time_begin, "begin"),
            soci::use(time_end,   "end"),
            soci::use(memo,   "memo"),
            soci::use(record_id,  "id");
    } catch (const soci::soci_error& e) {
        std::cerr << "UpdateRecord Error: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::GetAllCategories(std::vector<Category> &out){ // 全カテゴリ取得
	out.clear(); // 安全のため最初に初期化

    if (sql.get_backend() == nullptr) return false;

    try {
	soci::rowset<soci::row> rs = (sql.prepare <<
	    "SELECT id, parent_id, name, is_folder FROM categories WHERE is_hidden = 0 ORDER BY is_folder DESC, name ASC;");

	for (auto it = rs.begin(); it != rs.end(); ++it) {
	    const soci::row& row = *it;
	    Category c;

	    c.id = row.get<int>(0);
	    c.parent_id = row.get<int>(1);
	    c.name = row.get<std::string>(2);
	    c.is_folder = (row.get<int>(3) != 0);

	    out.push_back(c);
	    }
	return true;
    } catch (const soci::soci_error& e) {
	std::cerr << "GetAllCategories Error: " << e.what() << std::endl;
        return false;
    }
}

int Database::GetParentId(int id) {
    int parent_id = 0;
    try {
        sql << "SELECT parent_id FROM categories WHERE id = :id",
            soci::use(id),
            soci::into(parent_id);
    } catch (const soci::soci_error& e) {
	std::cerr << "GetParentId Error: " << e.what() << std::endl;
	return 0;
    }
    return parent_id;
}

bool Database::UpdateCategories(int id, const std::string& name){
	if (db == nullptr){
		return false;
	}

	const char* sql = "UPDATE categories SET name = ? WHERE id = ?;";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
		return false;
	}

	sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, id);

	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE){
		sqlite3_finalize(stmt);
		return false;
	}

	sqlite3_finalize(stmt);
	return true;
}

bool Database::HasRecords(int category_id) { // records の category_id で記録の有無判定
	if (db == nullptr) return false;

	const char* sql = "SELECT COUNT(*) FROM records WHERE category_id = ?;";
	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_int(stmt, 1, category_id);

	int count = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
	count = sqlite3_column_int(stmt, 0);
	}

	sqlite3_finalize(stmt);
	return count > 0;
}

bool Database::HideCategory(int id) {
	if (db == nullptr) return false;

	const char* sql = "UPDATE categories SET is_hidden = 1 WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_int(stmt, 1, id);

	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return rc == SQLITE_DONE;
}

bool Database::IsFolder(int id) {
    if (sql.get_backend() == nullptr) return false;

    try {
	int is_folder = 0;
	sql << "SELECT is_folder FROM categories WHERE id = :id",
	    soci::use(id), soci::into(is_folder);

	return (is_folder == 1);
    } catch (const soci::soci_error& e) {
	return false;
    }
}

bool Database::EditParentId(long long id, long long parent_id) {
	if (db == nullptr) return false;

	const char* sql = "UPDATE categories SET parent_id = ? WHERE id = ?;";
	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_int64(stmt, 1, parent_id);
	sqlite3_bind_int64(stmt, 2, id);

	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return rc == SQLITE_DONE;
}

// ISO8601 UTC文字列 "YYYY-MM-DDTHH:MM:SSZ" を
// エポック秒（UTC基準）に変換するヘルパ
static std::time_t parse_utc_iso8601(const std::string& s) {
	if (s.size() < 19) return 0;
	std::tm tm = {};
	tm.tm_year = std::stoi(s.substr(0, 4)) - 1900;
	tm.tm_mon  = std::stoi(s.substr(5, 2)) - 1;
	tm.tm_mday = std::stoi(s.substr(8, 2));
	tm.tm_hour = std::stoi(s.substr(11, 2));
	tm.tm_min  = std::stoi(s.substr(14, 2));
	tm.tm_sec  = std::stoi(s.substr(17, 2));
    // プラットフォーム非依存のUTC変換
#ifdef _WIN32
	return _mkgmtime(&tm);
#else
	return timegm(&tm);
#endif
}

bool Database::GetRecordsByDate(
        const std::string& local_date,
        int offset_seconds,
        std::vector<RecordSummary>& out)
{
	out.clear();
	if (db == nullptr) return false;

	// ローカル日付の00:00〜23:59をUTCに換算した前後1日を検索範囲にする
	// 例: JST(+9) の 3/16 → UTC換算で 3/15 も含めて取得
	// substr で前日・当日の2日分を指定
	std::string year  = local_date.substr(0, 4);
	std::string month = local_date.substr(5, 2);
	std::string day   = local_date.substr(8, 2);

	// UTC前日の日付文字列を作る (簡易: エポック秒経由)
	std::tm tm_local = {};
	tm_local.tm_year = std::stoi(year) - 1900;
	tm_local.tm_mon  = std::stoi(month) - 1;
	tm_local.tm_mday = std::stoi(day);

#ifdef _WIN32
	std::time_t t_local_midnight = _mkgmtime(&tm_local);
#else
	std::time_t t_local_midnight = timegm(&tm_local);
#endif

	// ローカル深夜0時のUTC = ローカル深夜0時 - オフセット
	std::time_t t_utc_start = t_local_midnight - offset_seconds;
	std::tm* tm_utc_prev = std::gmtime(&t_utc_start);
	char buf[11];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm_utc_prev);
	std::string utc_prev_date = buf; // UTC換算の前日 (または当日)

	const char* sql =
		"SELECT c.name, r.time_begin, r.time_end "
		"FROM records r "
		"JOIN categories c ON r.category_id = c.id "
		"WHERE substr(r.time_begin, 1, 10) >= ? "
		"AND substr(r.time_begin, 1, 10) <= ? "
		"ORDER BY c.name ASC;";

	sqlite3_stmt* stmt = nullptr;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		return false;
	}

	sqlite3_bind_text(stmt, 1, utc_prev_date.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, local_date.c_str(), -1, SQLITE_TRANSIENT);

	// ローカル日付の開始・終了エポック秒
	std::time_t local_day_start = t_local_midnight - offset_seconds; // UTC基準
	std::time_t local_day_end   = local_day_start + 86400;

	std::map<std::string, int> summary_map;

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* name_raw  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		const char* begin_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		const char* end_raw   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		if (!name_raw || !begin_raw || !end_raw) continue;

		std::string name = name_raw;
		std::time_t t_begin = parse_utc_iso8601(begin_raw);
		std::time_t t_end   = parse_utc_iso8601(end_raw);

		// ローカル日付の範囲にクリップ
		std::time_t clipped_begin = std::max(t_begin, local_day_start);
		std::time_t clipped_end   = std::min(t_end,   local_day_end);

		if (clipped_end <= clipped_begin) continue; // この日に含まれない

		int seconds = static_cast<int>(clipped_end - clipped_begin);
		summary_map[name] += seconds;
    }

	sqlite3_finalize(stmt);

	for (const auto& kv : summary_map) {
		RecordSummary rs;
		rs.category_name = kv.first;
		rs.total_seconds = kv.second;
		out.push_back(rs);
	}

	return true;
}

// カテゴリパスの取得．
// ID を渡したら，std::string のパス形式でreturnする．
std::string Database::GetCategoriesPath(int id) {
	
	if (db == nullptr) return "";

	std::vector<std::string> path_components;
	int current_id = id;
	int safety_limit = 20; // 無限探索阻止用の変数

	while (current_id > 0 && safety_limit > 0) {
		safety_limit--;
		// ID から，カテゴリ名とparent_idを取得
		const char* sql = "SELECT name, parent_id FROM categories WHERE id = ?;";
		sqlite3_stmt* stmt = nullptr;

		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		break;
		}

		sqlite3_bind_int(stmt, 1, current_id);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			// カテゴリ名格納 
			const char* name_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			// 親ID 格納
			int parent_id = sqlite3_column_int(stmt, 1);

			if (name_raw) {
				// 取得したカテゴリ名を push_back
				path_components.push_back(name_raw);
			}

			// 親ID = 0 or 自分自身が親のデータ（異常値）のとき終了
			if (parent_id == 0 || parent_id == current_id) {
				sqlite3_finalize(stmt);
				break;
			}
			current_id = parent_id;
		} else {
			sqlite3_finalize(stmt);
			break;
		}
		sqlite3_finalize(stmt);
	}

	if (path_components.empty()) {
		return "";
	}

	// 子，親，祖父 -> 祖父，親，子 に変換
	std::reverse(path_components.begin(), path_components.end());

	// 文字列を "/" で結合
	std::string full_path;
	for (int i = 0; i < path_components.size(); ++i) {
		full_path += path_components[i];
		if (i != path_components.size() - 1) {
			full_path += "/";
		}
	}
	return full_path;
}

// 最終実行時間を取得
long long Database::GetLastExecuted(int category_id) {
    if (sql.get_backend() == nullptr) return false;

    try {
	// 時間に long long を使っているのは、2038 年のオーバーフローに対応するため
	// 加えて、Streak 計算にも使うから、long long にしてる
	long long result = 0;

	sql <<
	"SELECT strftime('%s', MAX(time_begin)) FROM records WHERE category_id = :id",
	soci::use(category_id), soci::into(result);

	return result;
    } catch (const soci::soci_error& e) {
	std::cerr << "GetLastExecuted Error: " << e.what() << std::endl;
        return false;
    }
}

int Database::GetCategoryIdByRecordId(int record_id) {
    if (sql.get_backend() == nullptr) return -1;

    int result = 0;

    try {
	sql <<
	"SELECT category_id FROM records WHERE id = :id",
	soci::use(record_id), soci::into(result);
    } catch (const soci::soci_error& e) {
	std::cerr << "GetCategoryIdByRecordId Error: " << e.what() << std::endl;
        return -1;
    }
    return result;
}

std::string Database::GetCategoryName(int category_id) {
    if (sql.get_backend() == nullptr) return "";

    std::string result = "";

    try {
	sql <<
	"SELECT name FROM categories WHERE id = :id",
	soci::use(category_id), soci::into(result);
    } catch (const soci::soci_error& e) {
	std::cerr << "GetCategoryName Error: " << e.what() << std::endl;
        return "";
    }
    return result;
}

std::string Database::GetMemoByRecordId(int record_id) {
    if (sql.get_backend() == nullptr) return "";

    std::string result = "";

    try {
	sql <<
	"SELECT memo FROM records WHERE id = :id",
	soci::use(record_id), soci::into(result);
    } catch (const soci::soci_error& e) {
	std::cerr << "GetMemoByRecordId Error: " << e.what() << std::endl;
        return "";
    }
    return result;
}

Database::Record Database::GetTimeByRecordId(int record_id) {
    if (sql.get_backend() == nullptr) return {};

    Database::Record result;
    std::string begin_str, end_str;

    try {
	sql <<
	"SELECT strftime('%s', time_begin), strftime('%s', time_end) FROM records WHERE id = :id",
	soci::use(record_id),
	soci::into(begin_str),
	soci::into(end_str);
	result.time_begin = std::stoll(begin_str);
	result.time_end = std::stoll(end_str);
    } catch (const soci::soci_error& e) {
	std::cerr << "GetTimeByRecordId Error: " << e.what() << std::endl;
        return {};
    }
    return result;
}

void Database::Close() { // 閉じる
	if (db != nullptr) {
		sqlite3_close_v2(db);
		db = nullptr;
	}
    sql.close();
}
