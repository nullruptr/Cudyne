#include <wx/wx.h>
#include "goal.hpp"
#include "goal_text.hpp"
#include "edit_goal_dlg.hpp"
#include "core/goal/goal_eval.hpp"
#include "core/goal/goal_status.hpp"
#include "core/utils/format_time.hpp"
#include <algorithm>
#include <ctime>

namespace {

// Condition (All / Achieved / Unachieved / Invalid) に状態が当てはまるか
bool MatchesCondition(int condition, GoalEval::State s) {
    using GoalStatus::Group;
    switch (condition) {
    case 1: return GoalStatus::GroupOf(s) == Group::Achieved;
    case 2: return GoalStatus::GroupOf(s) == Group::Unachieved;
    case 3: return GoalStatus::GroupOf(s) == Group::Invalid;
    default: return true; // All
    }
}

// DB から読み直す間隔(秒)。記録の開始・終了など、タイマーの再計算では分からない変化を拾う
constexpr int kReloadIntervalSec = 30;

// 読み込み時点の値から、now 時点の状態と表示を再計算する (DB には問い合わせない)
void RecomputeRow(GoalRow& row, long long now) {
    // 記録中なら、読み込み後に経過した時間ぶん、実績が増えているものとする
    long long achieved = row.achieved_base;
    if (row.recording) {
        const long long upto = row.window_end == GoalEval::kNone ? now : std::min(now, row.window_end);
        if (upto > row.base_now) achieved += upto - row.base_now;
    }

    const GoalEval::Result res = GoalEval::Evaluate(row.spec, now, now, achieved);

    row.state = res.state;
    row.cells[Col_Status]    = GoalStateText(res.state);
    row.cells[Col_Achieved]  = TimeUtils::FormatSeconds(res.achieved);
    row.cells[Col_Remaining] = TimeUtils::FormatSeconds(res.remaining);
    row.cells[Col_Slack]     = res.has_slack ? GoalDurationText(res.slack) : wxString("-");
}

} // namespace


Goal::Goal(wxWindow* parent, Database &dbRef)
	: wxFrame(parent, wxID_ANY, wxT("Goal"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , m_db(dbRef)
    , m_timer(this) {

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1550, 600)));

    wxPanel* main_panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* st_choiceCondition = new wxStaticText(main_panel, wxID_ANY, _("Condition: "));

	m_choiceCondition = new wxChoice(main_panel, wxID_ANY);
	m_choiceCondition->Append(_("All"));
    m_choiceCondition->Append(_("Achieved"));
	m_choiceCondition->Append(_("Unachieved"));
	m_choiceCondition->Append(_("Invalid"));
	m_choiceCondition->SetSelection(2); // 初期表示は Unachieved

    m_btn_add = new wxButton(main_panel, wxID_ANY, _("Add"));
    m_btn_edit = new wxButton(main_panel, wxID_ANY, _("Edit"));
    m_btn_delete = new wxButton(main_panel, wxID_ANY, _("Delete"));
    m_btn_refresh = new wxButton(main_panel, wxID_ANY, _("Refresh"));

    ctrl_sizer->Add(st_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(m_btn_add, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(20));
    ctrl_sizer->Add(m_btn_edit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_btn_delete, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_btn_refresh, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

    sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));

    m_ctrl = new wxDataViewCtrl(main_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    // 列を追加する前に、モデルを関連付ける (モデルなしで列を足すとクラッシュする)
    m_model = new GoalListModel();
    m_ctrl->AssociateModel(m_model.get());

    // wxDATAVIEW_COL_SORTABLE は後で付けるかも

    // 列番号は GoalColumn (goal_list_model.hpp) を使う。モデルの値の番号と共通
    m_ctrl->AppendTextColumn(_("Status"),
                 Col_Status,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Name"),
                 Col_Name,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Category / ToDo"),
                 Col_Subject,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Period"),
                 Col_Period,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Target"),
                 Col_Target,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_RIGHT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Achieved"),
                 Col_Achieved,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_RIGHT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Remaining"),
                 Col_Remaining,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_RIGHT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Start Within"),
                 Col_Slack,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_RIGHT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Start"),
                 Col_Start,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("End"),
                 Col_End,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Active"),
                 Col_Active,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_CENTER,
                 wxDATAVIEW_COL_RESIZABLE);

    sizer->Add(m_ctrl, 1, wxEXPAND | wxALL, FromDIP(10));
    main_panel->SetSizer(sizer);
    CenterOnParent();

    // 評価のやり直しは更新ボタンのみ。フィルタの切り替えは、読み込み済みの行を絞り込むだけ
    m_btn_refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Reload(); });
    m_choiceCondition->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ApplyFilter(); });

    m_btn_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddGoal(); });
    m_btn_edit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EditSelectedGoal(); });
    m_btn_delete->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteSelectedGoal(); });
    m_ctrl->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent&) { EditSelectedGoal(); }); // ダブルクリック
    m_ctrl->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent&) { UpdateEditButton(); });

    Reload();
    FitColumns(); // 行を読み込んだあとの内容に合わせる

    Bind(wxEVT_TIMER, &Goal::OnTimer, this);
    m_timer.Start(1000); // 1 秒ごとに残り時間を再計算する

    Bind(wxEVT_DPI_CHANGED, &Goal::OnDpiChanged, this);
}

void Goal::OnDpiChanged(wxDPIChangedEvent& event) {
    event.Skip(); // ウィンドウサイズの調整など、既定の処理も行う

    // DPI の切り替えが落ち着いてから、列幅を合わせ直す
    CallAfter([this]() { FitColumns(); });
}

void Goal::FitColumns() {
    for (unsigned i = 0; i < m_ctrl->GetColumnCount(); ++i) {
        wxDataViewColumn* col = m_ctrl->GetColumn(i);
        if (col) col->SetWidth(wxCOL_WIDTH_AUTOSIZE);
    }
}

// 新規作成。保存されたら読み直す
void Goal::AddGoal() {
    EditGoalDlg dlg(this, m_db);
    if (dlg.ShowModal() == wxID_OK) Reload();
}

// 選択中の行の goal を編集する。保存されたら読み直す
void Goal::EditSelectedGoal() {
    const wxDataViewItem item = m_ctrl->GetSelection();
    if (!item.IsOk()) return;

    const int goal_id = m_model->GetGoalId(m_model->GetRow(item));
    if (goal_id == -1) return;

    EditGoalDlg dlg(this, m_db, -1, -1, goal_id);
    if (dlg.ShowModal() == wxID_OK) Reload();
}

// 選択中の goal を、確認のうえ削除する
void Goal::DeleteSelectedGoal() {
    const wxDataViewItem item = m_ctrl->GetSelection();
    if (!item.IsOk()) return;

    const int goal_id = m_model->GetGoalId(m_model->GetRow(item));
    if (goal_id == -1) return;

    if (wxMessageBox(_("Delete the selected goal?"), _("Confirm"),
                     wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES) {
        return;
    }
    if (!m_db.DeleteGoal(goal_id)) {
        wxMessageBox(_("Failed to delete"), "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    Reload();
}

void Goal::UpdateEditButton() {
    const bool selected = m_ctrl->GetSelection().IsOk();
    m_btn_edit->Enable(selected);
    m_btn_delete->Enable(selected);
}

// DB から全 goal を読み直して評価し、一覧を更新する
void Goal::Reload() {
    std::vector<GoalRow> rows;
    const long long now = static_cast<long long>(std::time(nullptr));

    for (const Database::Goal& g : m_db.GetAllGoals()) {
        GoalRow row;
        row.goal_id = g.goal_id;

        // 今の窓の実績を集計して評価する (窓の終点が今より先なら、今まで)
        const GoalStatus::Evaluated ev = GoalStatus::EvaluateNow(m_db, g, now);
        row.spec = ev.spec;
        row.achieved_base = ev.total.seconds;
        row.recording = ev.total.recording;
        row.base_now = now;
        row.window_end = ev.window.end;
        RecomputeRow(row, now);

        row.cells[Col_Name]   = wxString::FromUTF8(g.goal_name);

        if (g.category_id != -1) {
            row.cells[Col_Subject] = wxString::FromUTF8(m_db.GetCategoriesPath(g.category_id));
        } else if (g.todo_id != -1) {
            row.cells[Col_Subject] = wxString::FromUTF8(m_db.GetTodoById(g.todo_id).todo_name);
        } else {
            row.cells[Col_Subject] = "-";
        }

        row.cells[Col_Period] = GoalPeriodText(g);
        row.cells[Col_Target] = TimeUtils::FormatSeconds(g.target_time);
        row.cells[Col_Start]  = TimeUtils::FormatUTCStringToLocal(g.start_time);
        row.cells[Col_End]    = g.end_time.empty() ? wxString("-") : TimeUtils::FormatUTCStringToLocal(g.end_time);
        row.cells[Col_Active] = g.is_active == 1 ? "Yes" : "No";

        rows.push_back(std::move(row));
    }

    m_all_rows = std::move(rows);
    m_ticks_since_reload = 0;
    ApplyFilter();
}

// 読み込み済みの行を、Condition で絞り込んで表示する
void Goal::ApplyFilter() {
    const int condition = m_choiceCondition->GetSelection();

    std::vector<GoalRow> shown;
    for (const GoalRow& row : m_all_rows) {
        if (MatchesCondition(condition, row.state)) shown.push_back(row);
    }

    if (m_model->HasSameIds(shown)) {
        m_model->UpdateRows(std::move(shown)); // 並びが同じなら、選択を保ったまま値だけ更新
    } else {
        m_model->SetRows(std::move(shown));
    }
    UpdateEditButton();
}

// 1 秒ごと。DB には問い合わせず、読み込み時点の値から残り時間などを再計算する
void Goal::OnTimer(wxTimerEvent& WXUNUSED(event)) {
    const long long now = static_cast<long long>(std::time(nullptr));

    // 窓が終わった (日付が変わった等) 行がある、または一定時間ごとに、DB から読み直す
    ++m_ticks_since_reload;
    bool window_ended = false;
    for (const GoalRow& row : m_all_rows) {
        if (row.window_end != GoalEval::kNone && row.window_end > row.base_now && now >= row.window_end) {
            window_ended = true;
            break;
        }
    }
    if (window_ended || m_ticks_since_reload >= kReloadIntervalSec) {
        Reload();
        return;
    }

    for (GoalRow& row : m_all_rows) RecomputeRow(row, now);
    ApplyFilter();
}
