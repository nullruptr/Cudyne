#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "core/plan/plan_feasibility.hpp"

// Plan の下画面。設定した予定で、Goal を達成できるかの判定結果を、Goal の窓ごとに一覧で表示する
class PlanCheckPanel : public wxPanel {
public:
    explicit PlanCheckPanel(wxWindow* parent);

    // note: 判定した範囲の説明など、一覧の上に出す文
    void SetItems(const std::vector<PlanFeasibility::Item>& items, const wxString& note);
    void SetBusy(); // 再計算を待っている間の表示

private:
    wxStaticText* m_st_note;
    wxStaticText* m_st_summary;
    wxListCtrl* m_list;
};
