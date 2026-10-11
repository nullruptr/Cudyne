#pragma once
#include <wx/event.h>
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/dateevt.h>
#include <wx/dataview.h>
#include <wx/object.h>
#include <vector>
#include "core/db/database.hpp"
#include "goal_list_model.hpp"

class Goal : public wxFrame{
public:
	Goal(wxWindow* parent, Database& db);
private:
	Database &m_db;
	wxChoice* m_choiceCondition;
	wxDatePickerCtrl* m_dp_filter_date; // この日付以降の締切のものを表示する(超過分は常に表示)
	wxButton* m_btn_add;
	wxButton* m_btn_edit;
	wxButton* m_btn_refresh; // 押したときだけ評価をやり直す(自動更新はしない)

	wxDataViewCtrl* m_ctrl;
	wxObjectDataPtr<GoalListModel> m_model;
	std::vector<GoalRow> m_all_rows; // Reload で評価した全行(フィルタ前)

	void Reload();      // DB から全 goal を読み直して評価し、一覧を更新
	void ApplyFilter(); // m_all_rows を Condition で絞り込んで表示
	void AddGoal();            // 新規作成ダイアログを開く
	void EditSelectedGoal();   // 選択中の goal の編集ダイアログを開く
	void UpdateEditButton();   // 行が選択されているときだけ Edit を有効にする
};
