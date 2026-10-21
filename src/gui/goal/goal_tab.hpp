#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "core/db/database.hpp"

// Detail の Goal タブ。Category に設定された goal の窓を、期間ぶん一覧で表示する。
// 期間を絞らないとき(filtering = false)は、今の窓だけを表示する
class GoalTab : public wxPanel {
public:
    GoalTab(wxWindow* parent, Database& db, int category_id);
    void Load(bool filtering, const wxDateTime& from, const wxDateTime& to); // to の日を含む

private:
    Database& m_db;
    int m_category_id;
    wxListCtrl* m_list;
    wxStaticText* m_st_note;
};
