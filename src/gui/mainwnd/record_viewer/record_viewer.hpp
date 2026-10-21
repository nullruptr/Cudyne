#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/record_list/record_list_panel.hpp"

// record の一覧。既定は全カテゴリ。SetCategoryId で、指定カテゴリ以下に絞れる
class RecordViewer : public wxPanel {
public:
    RecordViewer(wxWindow* parent, Database& dbRef);
    void SetPeriodAndRefresh(const wxDateTime& start, const wxDateTime& end);
    void SetCategoryId(int category_id); // -1 = すべて。設定後は、次の SetPeriodAndRefresh から反映される
private:
    Database&        m_db;
    RecordListPanel* m_panel;
    int              m_category_id = -1;
};
