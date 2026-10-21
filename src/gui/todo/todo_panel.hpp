#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/timer.h>
#include <vector>
#include "core/db/database.hpp"

// ToDo の一覧と操作 (追加・編集・ソート・カウントダウン・記録開始)。
// ToDo ウィンドウと、Detail の ToDo タブで共通に使う。
//
// - category_id が -1 以外のときは、そのカテゴリ以下(自分自身と配下の項目)の ToDo だけを表示する。
// - external_period が false のときは、自前の「Show From」(この日以降の締切のものを表示。超過分は常に表示)で絞る。
//   true のときは「Show From」を出さず、SetPeriod で渡された期間で絞る
//   (ToDo の日付 = 開始・目標終了・締切・完了 の範囲が期間と重なるものを表示)。
class ToDoPanel : public wxPanel {
public:
	// command_target: 「Start Recording」のイベントを送る先 (メインウィンドウ)。nullptr ならアプリのトップウィンドウ
	ToDoPanel(wxWindow* parent, Database& db, int category_id = -1, bool external_period = false,
	          wxWindow* command_target = nullptr);

	// external_period = true のときに、表示する期間を渡す。filtering = false なら期間では絞らない。to の日を含む
	void SetPeriod(bool filtering, const wxDateTime& from, const wxDateTime& to);

private:
	Database &m_db;
	int m_category_id;
	bool m_external_period;
	wxWindow* m_command_target;

	wxListCtrl* m_list;
	wxChoice* m_choiceCondition;
	wxDatePickerCtrl* m_dp_filter_date; // external_period = false のときだけ使う
	wxButton* m_btn_add;
	wxButton* m_btn_edit;
	wxStaticText* m_st_count;           // "表示件数 / 全件数"
	wxTimer m_timer; // 締切等までのカウントダウンをリアルタイム更新する

	bool m_filtering = false;           // external_period = true のときの期間
	wxDateTime m_from;
	wxDateTime m_to;

	std::vector<Database::ToDo> m_todo_cache; // 現在表示中の内容。m_list の行と添字が対応する
	int m_sort_column = -1;
	bool m_sort_ascending = true;

	void RefreshList(); // DB へ再取得しに行き、フィルタ・ソートをかけて再描画する
	void RenderFromCache(); // m_todo_cache の内容で m_list を再構築する(DB へは行かない)
	void ApplySort(); // m_sort_column/m_sort_ascending に従い m_todo_cache をソートする
	wxColour ComputeRowColor(const Database::ToDo& todo, long long now_epoch) const;

	void OnConditionChanged(wxCommandEvent& WXUNUSED(event));
	void OnFilterDateChanged(wxDateEvent& WXUNUSED(event));
	void OnAdd(wxCommandEvent& WXUNUSED(event));
	void OnEdit(wxCommandEvent& WXUNUSED(event));
	void OnListDoubleClick(wxListEvent& WXUNUSED(event));
	void OnColumnClick(wxListEvent& event);
	void OnTimer(wxTimerEvent& WXUNUSED(event));
	void OnContextMenu(wxContextMenuEvent& event);
	void OnStartRecord(wxCommandEvent& WXUNUSED(event));
};

enum {
	ID_TODO_START_RECORD = wxID_HIGHEST + 100
};
