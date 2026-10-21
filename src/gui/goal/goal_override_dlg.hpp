#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <vector>
#include "core/db/database.hpp"
#include "core/goal/goal_eval.hpp"

// goal の、窓ごとの目標時間の例外設定 (免除を含む)。設定済みの窓を一覧で見て、追加・更新・削除ができる
class GoalOverrideDlg : public wxDialog {
public:
    GoalOverrideDlg(wxWindow* parent, Database& db, int goal_id);

private:
    Database& m_db;
    Database::Goal m_goal;
    GoalEval::GoalSpec m_spec;

    wxCheckBox* m_cb_filter;           // ON のとき、表示する窓を期間で絞る (OFF = すべて表示)
    wxDatePickerCtrl* m_dp_from;
    wxDatePickerCtrl* m_dp_to;
    wxStaticText* m_st_count;          // "表示件数 / 全件数"

    wxListCtrl* m_list;
    wxDatePickerCtrl* m_dp_date;      // この日を含む窓が対象になる
    wxStaticText* m_st_window;         // 対象の窓の範囲、または選べない理由
    wxSpinCtrlDouble* m_sc_value;
    wxChoice* m_ch_unit;               // seconds / minutes / hour
    wxTextCtrl* m_tc_memo;
    wxButton* m_btn_set;
    wxButton* m_btn_delete;

    std::vector<Database::GoalWindow> m_rows; // 例外のある窓。m_list の行と添字が対応する
    GoalEval::Window m_window;                // 日付ピッカーが指す窓
    wxString m_window_error;                  // 空 = 選べる窓

    void Reload();                         // DB から読み直して、期間で絞り込んだ一覧を更新
    void UpdateFilterControls();           // 絞り込みのチェックに応じて日付ピッカーを有効化
    void LoadRowToForm(long index);        // 一覧で選んだ行を、入力欄に反映
    void UpdateWindow();                   // 日付ピッカーが指す窓を求めて表示
    void UpdateButtons();
    long GetSelectedIndex() const;         // 選択中の行 (なければ -1)
    long long GetValueSeconds() const;
    void SetValueSeconds(long long seconds); // 割り切れる最大の単位で表示する
    void OnSet(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
};
