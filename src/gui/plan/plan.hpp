#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/grid.h>
#include <wx/spinctrl.h>
#include <vector>
#include "core/db/database.hpp"

// Plan の編集表 (Excel のような編集)。仕様は core/plan/plan.md。
// 編集はまず表の中だけに溜め、Save でまとめて DB に反映する。Discard で破棄して読み直す。
// ウィンドウ(PlanFrame)や、Detail のタブに載せて使う
//
// 内部では、予定の行を m_rows(行モデル)で持ち、行の追加・削除・複製のたびに表を描き直す。
// 表には、予定のない日の灰色の行(空き日の行)も出せるが、それは描画のときだけ作る表示用の行で、m_rows には入らない。
class Plan : public wxPanel {
    public:
	Plan(wxWindow* parent, Database& db);

	bool HasUnsavedChanges() const;
	// 未保存の変更があれば、破棄してよいか確認する。破棄してよい(または変更がない)ときに true
	bool ConfirmDiscard();

    private:
	enum Column { COL_ID = 0, COL_CATEGORY, COL_PATH, COL_START, COL_END, COL_TOTAL, COL_MEMO, COL_COUNT };
	enum class RowState { Clean, Modified, New };
	enum ShiftUnit { UNIT_MINUTES = 0, UNIT_HOURS, UNIT_DAYS, UNIT_WEEKS, UNIT_MONTHS };

	struct PlanRow {
	    int id = -1;                       // -1 = まだ保存されていない行
	    int goal_id = -1;                  // 表には出さないが、更新で消さないよう保持する
	    RowState state = RowState::Clean;
	    bool select_after_render = false;  // 複製した行を、描画のあとで選択状態にする
	    wxString cell[COL_COUNT];          // 表のセルの文字列 (時刻はローカル)
	};

	// 絞り込みの設定。変更を取り消すときに、直前の状態へ戻すために保持する
	struct FilterState {
	    bool period = false;
	    wxDateTime from;
	    wxDateTime to;
	    int category_id = -1;
	};

	Database &m_db;
	wxGrid* m_grid;

	// 絞り込み
	wxCheckBox* m_cb_period;               // ON のとき、期間で絞る (OFF = すべて)
	wxDatePickerCtrl* m_dp_from;
	wxDatePickerCtrl* m_dp_to;
	wxStaticText* m_st_category;
	int m_category_filter = -1;            // -1 = すべてのカテゴリ
	wxCheckBox* m_cb_empty_days;           // ON のとき、予定のない日を灰色の行で出す
	FilterState m_applied;                 // 直近に読み込んだときの絞り込み

	// 複製
	wxSpinCtrl* m_sc_shift;                // 複製した予定を、元から何(単位)ずらすか。負なら過去へ
	wxChoice* m_ch_unit;
	wxCheckBox* m_cb_copy_memo;

	wxStaticText* m_st_status;

	// 選択中の start_time / end_time セルを、日付(カレンダー)・時刻・秒で編集する入力欄 (入力は Edit ToDo と同じ)
	wxStaticText* m_st_time_target;
	wxDatePickerCtrl* m_dp_time;
	wxTextCtrl* m_tc_hhmm;
	wxTextCtrl* m_tc_ss;
	wxButton* m_btn_time_now;
	int m_time_row = -1;                   // 入力欄が編集している表のセル。対象のセルでなければ -1
	int m_time_col = -1;
	bool m_applying_time = false;          // 確認ダイアログでフォーカスが動いても、二重に処理しないための印

	std::vector<PlanRow> m_rows;           // 予定の行。表の並び(開始時刻順)
	std::vector<int> m_grid_to_data;       // 表の行 -> m_rows の添字 (空き日の行は -1)
	std::vector<int> m_data_to_grid;       // m_rows の添字 -> 表の行
	std::vector<int> m_conflict_partner;   // m_rows の添字 -> 同じカテゴリで時間が重なる相手の添字 (重ならなければ -1)
	std::vector<int> m_deleted_ids;        // 保存するまで DB に残る、削除予定の予定 ID
	std::vector<PlanRow> m_clipboard;      // Ctrl+C でためた行

	// 読み込みと描画
	void LoadFromDb();                     // DB から読み直す (未保存の変更は消える)
	void CaptureGrid();                    // 表で編集した内容を m_rows に取り込む (行を増減する前に呼ぶ)
	void Render();                         // m_rows から表を描き直す
	void RenderRow(int grid_row, int data_index);
	void RenderEmptyDayRow(int grid_row, const wxString& date);
	static void ComputeDerived(Database& db, PlanRow& row); // パスと合計時間を、他のセルから求める
	void UpdateRowColor(int grid_row);
	void RecomputeConflicts();             // 同じカテゴリで時間が重なる行を見つける (同じ時刻も重なり)
	void RefreshAllColors();
	void UpdateStatus(const wxString& note = wxString());
	bool RowToItem(const PlanRow& row, Database::PlanItem& item, wxString& error, int& error_col);
	FilterState CurrentFilter() const;
	void ApplyFilterToControls(const FilterState& state);
	void UpdateFilterControls();

	// 行の操作
	int InsertSorted(const PlanRow& row);  // 開始時刻の順に挿入する。入れた位置を返す
	std::vector<int> SelectedDataIndices() const; // 選択中の行 (なければカーソルの行) の m_rows の添字
	void AddRow();
	void DeleteSelectedRows();
	void DuplicateRows(const std::vector<PlanRow>& source, int shift, int unit, bool copy_memo);
	void DuplicateSelected();
	void CopySelected();
	void PasteClipboard();
	void Save();
	void Discard();
	void MarkChanged(int data_index);
	void SelectCategory(int grid_row);
	void ChooseCategoryFilter();

	void CellValueChanged(int grid_row, int grid_col); // セルの値が変わったあとの更新 (パス・合計・重なり・色・状態)
	void SyncTimeEditor(int grid_row, int grid_col);   // 選択したセルの日時を入力欄に出す。対象外のセルなら無効にする
	void ApplyTimeFromControls();                      // 入力欄の日時を、選択中のセルに反映する
	void ApplyTimeValue(int grid_row, int grid_col, const wxDateTime& dt); // 日時をセルに反映する。値が変わらなければ何もしない
	void OpenTimeDialog();                             // Ctrl+E: 選択中の日時セルを、ダイアログで編集する
	void OnSelectCell(wxGridEvent& event);
	void OnCellChanging(wxGridEvent& event);
	void OnCellChanged(wxGridEvent& event);
	void OnCellDoubleClick(wxGridEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnFilterChanged();
};

// Plan ウィンドウ。中身は Plan。未保存のまま閉じるときは確認を出す
class PlanFrame : public wxFrame {
    public:
	PlanFrame(wxWindow* parent, Database& db);
    private:
	Plan* m_plan;
};
