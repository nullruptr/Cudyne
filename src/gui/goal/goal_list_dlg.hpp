#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "core/db/database.hpp"

// 対象 (Category または ToDo) に設定された goal の一覧。新規作成・編集・削除ができる
class GoalListDlg : public wxDialog {
public:
    GoalListDlg(wxWindow* parent, Database& db, int category_id = -1, int todo_id = -1);

private:
    Database& m_db;
    int m_category_id;
    int m_todo_id;

    wxListCtrl* m_list;
    wxButton* m_btn_edit;
    wxButton* m_btn_delete;
    std::vector<Database::Goal> m_goal_cache; // m_list の行と添字が対応する

    void Reload();                  // DB から読み直して一覧を更新
    void UpdateButtons();           // 選択の有無でボタンの有効/無効を切り替え
    long GetSelectedIndex() const;  // 選択中の行 (なければ -1)
    void OnNew(wxCommandEvent& event);
    void OnEdit(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
};
