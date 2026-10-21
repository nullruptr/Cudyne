#include "goal_override_dlg.hpp"
#include "core/goal/goal_status.hpp"
#include "core/utils/format_time.hpp"
#include <cmath>
#include <wx/dateevt.h>
#include <wx/sizer.h>

namespace {
enum Column { COL_WINDOW = 0, COL_NORMAL, COL_OVERRIDE, COL_MEMO };
enum Unit { UNIT_SECONDS = 0, UNIT_MINUTES, UNIT_HOURS };
}

GoalOverrideDlg::GoalOverrideDlg(wxWindow* parent, Database& dbRef, int goal_id)
    : wxDialog(parent, wxID_ANY, _("Target Overrides"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) {

    m_goal = m_db.GetGoalById(goal_id);
    m_spec = GoalStatus::ToSpec(m_goal);

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(720, 520)));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(5);

    const wxString name = m_goal.goal_name.empty() ? wxString::Format("#%d", m_goal.goal_id) : wxString::FromUTF8(m_goal.goal_name);
    sizer->Add(new wxStaticText(this, wxID_ANY, wxString::Format(_("Goal: %s"), name)), 0, wxALL, FromDIP(10));

    // 表示する期間の絞り込み (チェックを外すとすべて表示)
    const long date_picker_style = wxDP_DROPDOWN | wxDP_SHOWCENTURY; // カレンダーから選べる
    m_cb_filter = new wxCheckBox(this, wxID_ANY, _("Filter by period"));
    wxDateTime first_day = wxDateTime::Today();
    first_day.SetDay(1);
    wxDateTime last_day = first_day;
    last_day.SetToLastMonthDay();
    m_dp_from = new wxDatePickerCtrl(this, wxID_ANY, first_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_dp_to   = new wxDatePickerCtrl(this, wxID_ANY, last_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_st_count = new wxStaticText(this, wxID_ANY, "");
    wxBoxSizer* filter_sizer = new wxBoxSizer(wxHORIZONTAL);
    filter_sizer->Add(m_cb_filter, 0, wxALIGN_CENTER_VERTICAL);
    filter_sizer->Add(m_dp_from, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    filter_sizer->Add(new wxStaticText(this, wxID_ANY, wxT("~")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, gap);
    filter_sizer->Add(m_dp_to, 0, wxALIGN_CENTER_VERTICAL);
    filter_sizer->Add(m_st_count, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(15));
    sizer->Add(filter_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(10));

    // 設定済みの窓の一覧
    m_list =new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_list->AppendColumn(_("Window"), wxLIST_FORMAT_LEFT, FromDIP(300));
    m_list->AppendColumn(_("Normal"), wxLIST_FORMAT_RIGHT, FromDIP(90));
    m_list->AppendColumn(_("Override"), wxLIST_FORMAT_RIGHT, FromDIP(110));
    m_list->AppendColumn(_("Memo"), wxLIST_FORMAT_LEFT, FromDIP(150));
    sizer->Add(m_list, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(10));

    // 追加・更新の入力欄
    wxFlexGridSizer* grid = new wxFlexGridSizer(0, 2, FromDIP(8), FromDIP(10));
    grid->AddGrowableCol(1, 1);
    const wxSizerFlags labelFlags = wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL);

    m_dp_date = new wxDatePickerCtrl(this, wxID_ANY, wxDateTime::Today(), wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN | wxDP_SHOWCENTURY);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Date in the window")), labelFlags);
    grid->Add(m_dp_date, labelFlags);

    m_st_window = new wxStaticText(this, wxID_ANY, "-");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Window")), labelFlags);
    grid->Add(m_st_window, wxSizerFlags(1).Expand().Align(wxALIGN_CENTER_VERTICAL));

    m_sc_value = new wxSpinCtrlDouble(this, wxID_ANY, "0", wxDefaultPosition, FromDIP(wxSize(90, -1)), wxSP_ARROW_KEYS, 0, 1000000, 0, 1);
    m_sc_value->SetDigits(2);
    wxArrayString units;
    units.Add(_("seconds"));
    units.Add(_("minutes"));
    units.Add(_("hour"));
    m_ch_unit = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, units);
    m_ch_unit->SetSelection(UNIT_HOURS);
    wxBoxSizer* value_sizer = new wxBoxSizer(wxHORIZONTAL);
    value_sizer->Add(m_sc_value, 0, wxALIGN_CENTER_VERTICAL);
    value_sizer->Add(m_ch_unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    value_sizer->Add(new wxStaticText(this, wxID_ANY, _("(0 = exempt)")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Target time for the window")), labelFlags);
    grid->Add(value_sizer, labelFlags);

    m_tc_memo = new wxTextCtrl(this, wxID_ANY, "");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Memo")), labelFlags);
    grid->Add(m_tc_memo, wxSizerFlags(1).Expand().Align(wxALIGN_CENTER_VERTICAL));

    sizer->Add(grid, 0, wxEXPAND | wxALL, FromDIP(10));

    // ボタン
    m_btn_set = new wxButton(this, wxID_ANY, _("Set"));
    m_btn_delete = new wxButton(this, wxID_ANY, _("Delete"));
    wxButton* btn_close = new wxButton(this, wxID_ANY, _("Close"));
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->Add(m_btn_set, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(m_btn_delete, 0, wxALL, FromDIP(5));
    bottom_sizer->AddStretchSpacer(1);
    bottom_sizer->Add(btn_close, 0, wxALL, FromDIP(5));
    sizer->Add(bottom_sizer, 0, wxEXPAND);

    SetSizer(sizer);
    CenterOnParent();

    m_cb_filter->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdateFilterControls(); Reload(); });
    // From > To にならないよう、動かした側に合わせてもう一方を寄せる
    m_dp_from->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
        if (m_dp_from->GetValue() > m_dp_to->GetValue()) m_dp_to->SetValue(m_dp_from->GetValue());
        Reload();
    });
    m_dp_to->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
        if (m_dp_to->GetValue() < m_dp_from->GetValue()) m_dp_from->SetValue(m_dp_to->GetValue());
        Reload();
    });
    m_dp_date->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) { UpdateWindow(); });
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) {
        LoadRowToForm(GetSelectedIndex());
        UpdateButtons();
    });
    m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { UpdateButtons(); });
    m_btn_set->Bind(wxEVT_BUTTON, &GoalOverrideDlg::OnSet, this);
    m_btn_delete->Bind(wxEVT_BUTTON, &GoalOverrideDlg::OnDelete, this);
    btn_close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });

    UpdateFilterControls();
    Reload();
    UpdateWindow();
    UpdateButtons();
}

void GoalOverrideDlg::UpdateFilterControls() {
    const bool enabled = m_cb_filter->GetValue();
    m_dp_from->Enable(enabled);
    m_dp_to->Enable(enabled);
}

void GoalOverrideDlg::Reload() {
    // 絞り込みの期間 [from, to の翌日 0 時)。窓がこの期間と重なっていれば表示する
    const bool filtering = m_cb_filter->GetValue();
    long long from_epoch = 0, to_epoch = 0;
    if (filtering) {
        wxDateTime from = m_dp_from->GetValue();
        from.ResetTime();
        wxDateTime to = m_dp_to->GetValue();
        to.ResetTime();
        to.Add(wxDateSpan::Day());
        from_epoch = static_cast<long long>(from.GetTicks());
        to_epoch = static_cast<long long>(to.GetTicks());
    }

    m_rows.clear();
    size_t total = 0;
    for (const Database::GoalWindow& w : m_db.GetGoalWindows(m_goal.goal_id)) {
        if (w.target_override == -1) continue;
        ++total;

        if (filtering) {
            // 窓が求まらないキー(設定後に Start などが変わった等)は、キーの日付 1 点で判定する
            const long long at = GoalEval::WindowKeyToEpoch(w.window_key);
            const GoalEval::Window win = at == GoalEval::kNone ? GoalEval::Window() : GoalEval::GetWindow(m_spec, at);
            const long long win_start = win.valid ? win.start : at;
            const long long win_end = (win.valid && win.end != GoalEval::kNone) ? win.end : at + 1;
            if (at == GoalEval::kNone || !(win_start < to_epoch && win_end > from_epoch)) continue;
        }
        m_rows.push_back(w);
    }
    m_st_count->SetLabel(wxString::Format(_("%d / %d shown"), static_cast<int>(m_rows.size()), static_cast<int>(total)));

    m_list->DeleteAllItems();
    for (size_t i = 0; i < m_rows.size(); ++i) {
        const Database::GoalWindow& w = m_rows[i];

        wxString range = wxString::FromUTF8(w.window_key);
        const long long at = GoalEval::WindowKeyToEpoch(w.window_key);
        if (at != GoalEval::kNone) {
            const GoalEval::Window win = GoalEval::GetWindow(m_spec, at);
            if (win.valid && win.end != GoalEval::kNone) {
                range = TimeUtils::FormatEpochToDate(win.start) + " - " + TimeUtils::FormatEpochToDate(win.end);
            }
        }

        const long item = m_list->InsertItem(static_cast<long>(i), range);
        m_list->SetItem(item, COL_NORMAL, TimeUtils::FormatSeconds(m_goal.target_time));
        m_list->SetItem(item, COL_OVERRIDE, w.target_override == 0
            ? wxString(_("0 (exempt)"))
            : TimeUtils::FormatSeconds(w.target_override));
        m_list->SetItem(item, COL_MEMO, wxString::FromUTF8(w.memo));
    }
    UpdateButtons();
}

long GoalOverrideDlg::GetSelectedIndex() const {
    return m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void GoalOverrideDlg::UpdateButtons() {
    m_btn_set->Enable(m_window_error.IsEmpty());
    m_btn_delete->Enable(GetSelectedIndex() != -1);
}

void GoalOverrideDlg::LoadRowToForm(long index) {
    if (index < 0 || index >= static_cast<long>(m_rows.size())) return;
    const Database::GoalWindow& w = m_rows[index];

    const long long at = GoalEval::WindowKeyToEpoch(w.window_key);
    if (at != GoalEval::kNone) m_dp_date->SetValue(wxDateTime(static_cast<time_t>(at)));
    SetValueSeconds(w.target_override);
    m_tc_memo->SetValue(wxString::FromUTF8(w.memo));
    UpdateWindow();
}

// 日付ピッカーが指す窓を求める。窓が選べないときは、理由を表示してボタンを無効にする
void GoalOverrideDlg::UpdateWindow() {
    m_window = GoalEval::Window();
    m_window_error.Clear();

    if (!GoalEval::SupportsOverride(m_spec.period_type)) {
        m_window_error = _("This period type does not support target overrides.");
    } else {
        wxDateTime date = m_dp_date->GetValue();
        date.ResetTime();
        const long long at = static_cast<long long>(date.GetTicks());

        m_window = GoalEval::GetWindow(m_spec, at);
        const GoalEval::Window first = GoalEval::GetWindow(m_spec, m_spec.start_epoch);

        if (!m_window.valid) {
            m_window_error = _("No window exists on this date (before the start).");
        } else if (m_spec.end_epoch != GoalEval::kNone && at >= m_spec.end_epoch) {
            m_window_error = _("This date is after the End Time.");
        } else if (first.valid && m_window.start < first.start) {
            m_window_error = _("This window is before the start of the goal.");
        }
    }

    if (!m_window_error.IsEmpty()) {
        m_st_window->SetLabel(m_window_error);
        m_st_window->SetForegroundColour(*wxRED);
    } else {
        m_st_window->SetLabel(TimeUtils::FormatEpochToDate(m_window.start) + " - " + TimeUtils::FormatEpochToDate(m_window.end));
        m_st_window->SetForegroundColour(wxNullColour);
    }
    Layout();
    UpdateButtons();
}

long long GoalOverrideDlg::GetValueSeconds() const {
    const double value = m_sc_value->GetValue();
    switch (m_ch_unit->GetSelection()) {
    // 浮動小数点の誤差で 1 秒ずれないよう、四捨五入する
    case UNIT_MINUTES: return std::llround(value * 60);
    case UNIT_HOURS:   return std::llround(value * 3600);
    default:           return std::llround(value);
    }
}

void GoalOverrideDlg::SetValueSeconds(long long seconds) {
    if (seconds > 0 && seconds % 3600 == 0) {
        m_ch_unit->SetSelection(UNIT_HOURS);
        m_sc_value->SetValue(seconds / 3600.0);
    } else if (seconds > 0 && seconds % 60 == 0) {
        m_ch_unit->SetSelection(UNIT_MINUTES);
        m_sc_value->SetValue(seconds / 60.0);
    } else {
        m_ch_unit->SetSelection(UNIT_SECONDS);
        m_sc_value->SetValue(static_cast<double>(seconds));
    }
}

void GoalOverrideDlg::OnSet(wxCommandEvent& WXUNUSED(event)) {
    if (!m_window_error.IsEmpty()) return;

    const long long seconds = GetValueSeconds();
    const long long window_length = m_window.end - m_window.start;
    if (seconds > window_length) {
        wxMessageBox(_("Target time exceeds the length of the window"), "Error", wxOK | wxICON_WARNING, this);
        return;
    }

    // 最初の例外を作ると、窓の位置が変わる項目は変更できなくなる
    if (!m_db.HasGoalWindowOverride(m_goal.goal_id)) {
        const int answer = wxMessageBox(
            _("Once a target override exists, the Period Type, n and Start Time of this goal can no longer be changed "
              "(until all overrides are deleted). Continue?"),
            _("Confirm"), wxYES_NO | wxICON_QUESTION, this);
        if (answer != wxYES) return;
    }

    const std::string key = GoalEval::WindowKey(m_window.start);
    if (!m_db.SetGoalWindowOverride(m_goal.goal_id, key, seconds, m_tc_memo->GetValue().utf8_string())) {
        wxMessageBox(_("Failed to save"), "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    Reload();
}

void GoalOverrideDlg::OnDelete(wxCommandEvent& WXUNUSED(event)) {
    const long index = GetSelectedIndex();
    if (index < 0 || index >= static_cast<long>(m_rows.size())) return;

    if (wxMessageBox(_("Delete the selected override?"), _("Confirm"),
                     wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES) {
        return;
    }
    if (!m_db.ClearGoalWindowOverride(m_goal.goal_id, m_rows[index].window_key)) {
        wxMessageBox(_("Failed to delete"), "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    Reload();
}
