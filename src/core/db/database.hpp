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
	std::string memo;
	int         todo_id = 0;  // 0 = 紐付く ToDo なし
	std::string todo_name;
    };

    enum class ToDoFilter : int {
        All     = -1,
        Pending = 0,
        Done    = 1,
    };

    struct ToDo {
        int todo_id;
        int category_id;
        int status;
        int priority;
        std::string todo_name;
        std::string time_begin;
        std::string target_end;
        std::string deadline;
        std::string completion_date;
        std::string memo;
    };

	Database();
	~Database(); // 安全のため，Close()を呼んでる．
	bool Connect(const std::string& path); // 接続．
	bool Create(const std::string& path);// 新規作成．接続先パス
	bool Initialize(); // 初期化処理
	std::string GetDBPath() const;
	// --- 書き込み ---
	bool InsertCategories(int parent_id, const std::string &name, bool is_folder); // カテゴリ名，親ID，フォルダ
	bool InsertRecords(int category_id, const std::string &time_begin, const std::string &time_end, const std::string &memo, int todo_id = 0); // カテゴリテーブルでのID，開始時刻，終了時刻，メモ，ToDo ID(0=紐付けなし)
	bool UpdateRecords(int record_id, int category_id, const std::string &time_begin, const std::string &time_end, const std::string &memo, int todo_id = 0);
	struct Category{
		int id;
		int parent_id;
		std::string name;
		int is_folder;
	};
	bool GetAllCategories(std::vector<Category>& out);
	bool GetChildCategories(std::vector<int>& out, int id); // id 以下の is_folder = 0 の項目 id を全て取得
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
	int GetTodoIdByRecordId(int record_id); // Record ID から、ToDo ID を取得(0 = 紐付けなし)
	std::string GetCategoryName(int category_id); // Category ID から、名前を取得
	std::string GetMemoByRecordId(int record_id); // Record ID から、Memo を取得
	Database::Record GetTimeByRecordId(int record_id); // Record ID から、開始時刻と終了時刻を取得
    // --- ToDo ---
    bool InsertToDo(const ToDo& todo);
	bool UpdateToDo(const ToDo& todo);
	std::vector<Database::ToDo> GetTodoList(ToDoFilter filter = ToDoFilter::All);
	Database::ToDo GetTodoById(int todo_id);

	// --- Goal (db_goal.cpp) ---
	struct Goal {
		int goal_id = -1;
		int category_id = -1;  // -1 = NULL (category_id / todo_id はどちらか一方のみ設定)
		int todo_id = -1;      // -1 = NULL
		std::string goal_name;
		int period_type = 0;   // 0:DAILY, 1:WEEKLY, 2:MONTHLY, 3:EVERY_N_DAYS, 4:UNDEFINED(周期未定義), 5:ROLLING(達成起点リピート)
		int period_n = -1;     // -1 = NULL (period_type=3, 5 のときのみ)
		long long target_time = 0; // 期間ごとの目標(単位:sec)
		std::string start_time;    // 開始日時(UTC)
		std::string end_time;      // 終了日時(UTC)。空 = NULL (期限なし)
		int is_active = 1;
		std::string memo;
	};
	bool InsertGoal(const Goal& goal);
	bool UpdateGoal(const Goal& goal);
	Database::Goal GetGoalById(int goal_id); // 見つからなければ goal_id = -1
	std::vector<Database::Goal> GetGoalsByTarget(int category_id, int todo_id); // 対象(どちらか一方、使わない方は -1)に設定された goal を ID 順に列挙
	std::vector<Database::Goal> GetAllGoals(); // 全 goal を ID 順に列挙
	bool DeleteGoal(int goal_id); // goal_window の行も一緒に消える (外部キーの ON DELETE CASCADE)

	// --- goal_window (db_goal.cpp) ---
	struct GoalWindow {
		int id = -1;
		int goal_id = -1;
		std::string window_key;          // 通常型: 窓開始のローカル暦日 / Rolling: サイクル開始(UTC)
		long long target_override = -1;  // -1 = NULL (例外なし), 0 = 免除
		std::string achieved_at;         // Rolling のみ。達成時刻(UTC)。空 = NULL (未達)
		std::string memo;
	};
	std::vector<Database::GoalWindow> GetGoalWindows(int goal_id); // window_key 順
	bool SetGoalWindowAchieved(int goal_id, const std::string& window_key, const std::string& achieved_at); // なければ作る。target_override は保つ
	bool SetGoalWindowOverride(int goal_id, const std::string& window_key, long long target_override, const std::string& memo); // なければ作る。achieved_at は保つ
	bool ClearGoalWindowOverride(int goal_id, const std::string& window_key); // 例外だけを消す。他に持つ情報がなければ行ごと消す
	bool HasGoalWindowOverride(int goal_id); // 過去・現在を問わず、目標の例外設定が 1 件でもあるか
	// --- plan (db_plan.cpp) ---
	struct PlanItem {
		int id = -1;             // -1 = まだ保存されていない(新規)
		int goal_id = -1;        // -1 = NULL (Goal に紐づかない)。更新しても変えない
		int category_id = -1;
		std::string start_time;  // UTC "YYYY-MM-DD HH:MM:SS"
		std::string end_time;    // UTC
		std::string memo;
	};
	// end_time が from_utc より後の予定を、start_time 順に返す。to_utc が空でなければ、start_time がそれより前のものに絞る
	// category_id != -1 のときは、そのカテゴリ以下(自分自身と配下の項目)の予定に絞る
	std::vector<Database::PlanItem> GetPlans(const std::string& from_utc, const std::string& to_utc = "", int category_id = -1);
	// 予定として保存できるか。問題がなければ空文字、あれば理由を返す (時刻の形式、終了 > 開始、カテゴリが有効なタスクか)
	std::string ValidatePlanItem(const PlanItem& item);
	// 追加・更新・削除を、1 つのトランザクションでまとめて反映する。1 件でも問題があれば何も反映せず false (error に理由)。
	// created の id は無視する。updated の goal_id は変えない。新規の id は new_ids に created と同じ順で返す
	bool ApplyPlanChanges(const std::vector<PlanItem>& created, const std::vector<PlanItem>& updated,
	                      const std::vector<int>& deleted, std::string& error, std::vector<int>* new_ids = nullptr);
	void Close(); // DB クローズ処理
	
    // --- db_report.cpp ---
    std::vector<Database::RecordSummary> GetRecordsByRange(const std::string& start_utc, const std::string& end_utc);
    std::vector<Database::Record> GetRecordList(const std::string& start_utc, const std::string& end_utc, int category_id = -1); // category_id != -1 のときは、そのカテゴリ以下に絞る
    
	// --- db_record.cpp ---
	long long StartRecord(int category_id, int todo_id = 0); // todo_id が 0 以下の場合は ToDo と紐付けない
	bool EndRecord(int record_id);
    std::vector<Database::Record> GetUnfinishedRecords(); // 終了していない(time_end未設定の) record を列挙
	// --- db_total.cpp ---
	long long int GetTotalTime(int category_id, const std::string& start_utc, const std::string& end_utc);
	long long int GetTotalTimeByTodo(int todo_id, const std::string& start_utc, const std::string& end_utc); // todo_id に紐付く record の合計(秒)

private:
	sqlite3* db;
	soci::session sql;
	std::string path;
	std::string m_get_path;
};
