#pragma once
#include <soci/session.h>
#include <sqlite3.h>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    struct Record {
	int         id;
	int         category_id;
	std::string category_name;
	long long   time_begin;   // UTC epoch
	long long   time_end;     // UTC epoch
	long long   total_seconds;
    };

	Database();
	~Database(); // 安全のため，Close()を呼んでる．
	bool Connect(const std::string& path); // 接続．
	bool Create(const std::string& path);// 新規作成．接続先パス
	bool Initialize(); // 初期化処理
	std::string GetDBPath() const;
	// --- 書き込み ---
	bool InsertCategories(int parent_id, const std::string &name, bool is_folder); // カテゴリ名，親ID，フォルダ
	bool InsertRecords(int category_id, const std::string &time_begin, const std::string &time_end); // カテゴリテーブルでのID，開始時刻，終了時刻
	struct Category{
		int id;
		int parent_id;
		std::string name;
		int is_folder;
	};
	bool GetAllCategories(std::vector<Category>& out);
	int  GetParentId(int id);
	bool UpdateCategories(int id, const std::string& name);
	// ------
	bool HasRecords(int category_id);   // 記録の存在確認
	bool HideCategory(int id);          // ソフトデリート
	bool IsFolder(int id); // フォルダか否か判定
	bool EditParentId(long long id, long long parent_id); // ツリーアイテム移動

	struct RecordSummary {
		int category_id;
		std::string category_name;
		int total_seconds; // 合計秒数
	};

	bool GetRecordsByDate(const std::string& local_date,
                      int offset_seconds,
                      std::vector<RecordSummary>& out);

	std::string GetCategoriesPath(int id);
	long long GetLastExecuted(int category_id);
	int GetCategoryIdByRecordId(int record_id); // Record ID から、Category ID を取得
	std::string GetCategoryName(int category_id); // Category ID から、名前を取得
	std::string GetMemoByRecordId(int record_id); // Record ID から、Memo を取得
	Database::Record GetTimeByRecordId(int record_id); // Record ID から、開始時刻と終了時刻を取得
	void Close(); // DB クローズ処理
	
    // --- db_report.cpp ---
    std::vector<Database::RecordSummary> GetRecordsByRange(const std::string& start_utc, const std::string& end_utc);
    std::vector<Database::Record> GetRecordList(const std::string& start_utc, const std::string& end_utc, int category_id = -1);
    
	// --- db_record.cpp ---
	long long StartRecord(int category_id);
	bool EndRecord(int record_id);
	// --- db_total.cpp ---
	long long int GetTotalTime(int category_id, const std::string& start_utc, const std::string& end_utc);

private:
	sqlite3* db;
	soci::session sql;
	std::string path;
	std::string m_get_path;
};
