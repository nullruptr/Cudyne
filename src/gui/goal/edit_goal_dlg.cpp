#include "edit_goal_dlg.hpp"
#include "core/utils/format_time.hpp"
#include "gui/common/sel_category_dlg/sel_category_dlg.hpp"
#include "gui/common/sel_todo_dlg/sel_todo_dlg.hpp"
#include <cmath>
#include <wx/dateevt.h>
#include <wx/sizer.h>
#include <wx/wx.h>

namespace {
enum TargetKind { KIND_CATEGORY = 0, KIND_TODO = 1 };
enum PeriodType { PERIOD_DAILY = 0, PERIOD_WEEKLY, PERIOD_MONTHLY, PERIOD_EVERY_N_DAYS, PERIOD_UNDEFINED };
enum TargetUnit { UNIT_SECONDS = 0, UNIT_MINUTES, UNIT_HOURS };
}

EditGoalDlg::EditGoalDlg(wxWindow* parent, Database& dbRef, int category_id, int todo_id, int goal_id)
    : wxDialog(parent, wxID_ANY, _("Goal"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) {

    // 編集モード: 保存済みの goal を読み込む (見つからなければ新規として扱う)
    Database::Goal loaded;
    if (goal_id != -1) {
        loaded = m_db.GetGoalById(goal_id);
        if (loaded.goal_id != -1) {
            m_goal_id = loaded.goal_id;
            category_id = loaded.category_id;
            todo_id = loaded.todo_id;
        }
    }

    // 初期選択
    if (category_id != -1) {
        m_category_id = category_id;
    } else if (todo_id != -1) {
        m_todo_id = todo_id;
    }

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(520, 600)));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // 2列 (ラベル / 値)
    wxFlexGridSizer* grid = new wxFlexGridSizer(0, 2, FromDIP(8), FromDIP(10));
    grid->AddGrowableCol(1, 1);

    const wxSizerFlags labelFlags = wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL);
    const wxSizerFlags valueFlags = wxSizerFlags(1).Expand().Align(wxALIGN_CENTER_VERTICAL);
    const int gap = FromDIP(5);

    // Name
    m_tc_name = new wxTextCtrl(this, wxID_ANY, "");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Name (optional)")), labelFlags);
    grid->Add(m_tc_name, valueFlags);

    // Category / ToDo + Select
    wxArrayString kinds;
    kinds.Add(_("Category"));
    kinds.Add(_("ToDo"));
    m_ch_target_kind = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, kinds);
    m_ch_target_kind->SetSelection(m_todo_id != -1 ? KIND_TODO : KIND_CATEGORY);
    wxButton* btn_select = new wxButton(this, wxID_ANY, _("Select"));
    wxBoxSizer* kind_sizer = new wxBoxSizer(wxHORIZONTAL);
    kind_sizer->Add(m_ch_target_kind, 1, wxALIGN_CENTER_VERTICAL);
    kind_sizer->Add(btn_select, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Category / ToDo")), labelFlags);
    grid->Add(kind_sizer, valueFlags);

    // Selected Name
    m_st_selected_name = new wxStaticText(this, wxID_ANY, "-");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Selected Name")), labelFlags);
    grid->Add(m_st_selected_name, valueFlags);

    // Selected Path / ID
    m_st_selected_path = new wxStaticText(this, wxID_ANY, "-");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Selected Path")), labelFlags);
    grid->Add(m_st_selected_path, valueFlags);

    m_st_selected_id = new wxStaticText(this, wxID_ANY, "-");
    grid->Add(new wxStaticText(this, wxID_ANY, _("Selected ID")), labelFlags);
    grid->Add(m_st_selected_id, valueFlags);

    // Active
    m_cb_active = new wxCheckBox(this, wxID_ANY, _("Active"));
    m_cb_active->SetValue(true);
    grid->AddSpacer(0);
    grid->Add(m_cb_active, labelFlags);

    // Period Type
    wxArrayString periods;
    periods.Add(_("Daily"));
    periods.Add(_("Weekly"));
    periods.Add(_("Monthly"));
    periods.Add(_("Every n Days"));
    periods.Add(_("Undefined"));
    m_ch_period_type = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, periods);
    m_ch_period_type->SetSelection(PERIOD_DAILY);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Period Type")), labelFlags);
    grid->Add(m_ch_period_type, valueFlags);

    // Every n Days
    m_sc_every_n = new wxSpinCtrl(this, wxID_ANY, "3", wxDefaultPosition, FromDIP(wxSize(70, -1)), wxSP_ARROW_KEYS, 1, 365, 3);
    m_st_days = new wxStaticText(this, wxID_ANY, _("Days"));
    wxBoxSizer* every_n_sizer = new wxBoxSizer(wxHORIZONTAL);
    every_n_sizer->Add(m_sc_every_n, 0, wxALIGN_CENTER_VERTICAL);
    every_n_sizer->Add(m_st_days, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Every n Days")), labelFlags);
    grid->Add(every_n_sizer, labelFlags);

    // Start Time
    const long date_picker_style = wxDP_DROPDOWN | wxDP_SHOWCENTURY;
    m_dp_start      = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_start_hhmm = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_start_ss   = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);
    wxButton* btn_now = new wxButton(this, wxID_ANY, _("= Now"));
    wxBoxSizer* start_sizer = new wxBoxSizer(wxHORIZONTAL);
    start_sizer->Add(m_dp_start, 0, wxALIGN_CENTER_VERTICAL);
    start_sizer->Add(m_tc_start_hhmm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    start_sizer->Add(m_tc_start_ss, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    start_sizer->Add(btn_now, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Start Time")), labelFlags);
    grid->Add(start_sizer, labelFlags);

    // End Time (任意。チェックを外すと期限なし)
    m_cb_end        = new wxCheckBox(this, wxID_ANY, _("End Time"));
    m_dp_end        = new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_end_hhmm   = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_end_ss     = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);
    m_btn_end_now   = new wxButton(this, wxID_ANY, _("= Now"));
    wxBoxSizer* end_sizer = new wxBoxSizer(wxHORIZONTAL);
    end_sizer->Add(m_dp_end, 0, wxALIGN_CENTER_VERTICAL);
    end_sizer->Add(m_tc_end_hhmm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    end_sizer->Add(m_tc_end_ss, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    end_sizer->Add(m_btn_end_now, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(m_cb_end, labelFlags);
    grid->Add(end_sizer, labelFlags);

    // Target time
    m_sc_target_value = new wxSpinCtrlDouble(this, wxID_ANY, "60", wxDefaultPosition, FromDIP(wxSize(90, -1)), wxSP_ARROW_KEYS, 0, 1000000, 60, 1);
    m_sc_target_value->SetDigits(2); // 2.5 minutes などの小数入力を許可する
    wxArrayString units;
    units.Add(_("seconds"));
    units.Add(_("minutes"));
    units.Add(_("hour"));
    m_ch_target_unit = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, units);
    m_ch_target_unit->SetSelection(UNIT_MINUTES);
    m_st_target_max = new wxStaticText(this, wxID_ANY, "");
    wxBoxSizer* target_sizer = new wxBoxSizer(wxHORIZONTAL);
    target_sizer->Add(m_sc_target_value, 0, wxALIGN_CENTER_VERTICAL);
    target_sizer->Add(m_ch_target_unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    target_sizer->Add(m_st_target_max, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Target time")), labelFlags);
    grid->Add(target_sizer, labelFlags);

    // 目標時間が期間の長さを超えているときの警告 (赤字)
    m_st_target_warn = new wxStaticText(this, wxID_ANY, "");
    m_st_target_warn->SetForegroundColour(*wxRED);
    grid->AddSpacer(0);
    grid->Add(m_st_target_warn, labelFlags);

    // 親カテゴリの目標による制約
    m_st_constraint = new wxStaticText(this, wxID_ANY, _("NO"));
    m_btn_edit_parent_goal = new wxButton(this, wxID_ANY, _("Edit parent category's goal"));
    m_btn_edit_parent_goal->Enable(false); // 親カテゴリの目標が存在する場合のみ有効にする予定
    wxBoxSizer* constraint_sizer = new wxBoxSizer(wxHORIZONTAL);
    constraint_sizer->Add(m_st_constraint, 0, wxALIGN_CENTER_VERTICAL);
    constraint_sizer->Add(m_btn_edit_parent_goal, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(15));
    grid->Add(new wxStaticText(this, wxID_ANY, _("Constraints from parent category's goal")), labelFlags);
    grid->Add(constraint_sizer, labelFlags);

    sizer->Add(grid, 0, wxEXPAND | wxALL, FromDIP(10));

    // メモ
    m_tc_memo = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    sizer->Add(m_tc_memo, 1, wxALL | wxEXPAND, FromDIP(10));

    // ボタン (下部)
    wxButton* btn_save = new wxButton(this, wxID_ANY, _("Save"));
    wxButton* btn_cancel = new wxButton(this, wxID_ANY, _("Cancel"));
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
    bottom_sizer->AddStretchSpacer(1);
    bottom_sizer->Add(btn_save, 0, wxALL, FromDIP(5));
    bottom_sizer->Add(btn_cancel, 0, wxALL, FromDIP(5));
    sizer->Add(bottom_sizer, 0, wxEXPAND);

    SetSizer(sizer);
    CenterOnParent();

    // 初期値: Start Time は現在時刻
    wxDateTime now = wxDateTime::Now();
    m_dp_start->SetValue(now);
    m_tc_start_hhmm->SetValue(now.Format("%H:%M"));
    m_tc_start_ss->SetValue(now.Format("%S"));
    // End Time は未設定(期限なし)が既定。チェックを入れたときのために現在時刻を入れておく
    m_dp_end->SetValue(now);
    m_tc_end_hhmm->SetValue(now.Format("%H:%M"));
    m_tc_end_ss->SetValue(now.Format("%S"));

    // 編集モード: 保存済みの値を各コントロールに反映する
    if (m_goal_id != -1) {
        m_tc_name->SetValue(wxString::FromUTF8(loaded.goal_name));
        m_cb_active->SetValue(loaded.is_active == 1);
        m_ch_period_type->SetSelection(loaded.period_type);
        if (loaded.period_n != -1) m_sc_every_n->SetValue(loaded.period_n);

        // DB には UTC で保存されているので、システム(ローカル)時間に変換して表示する
        TimeUtils::ParsedTime start = TimeUtils::ParseUTCString(loaded.start_time);
        m_dp_start->SetValue(start.date);
        m_tc_start_hhmm->SetValue(start.hhmm);
        m_tc_start_ss->SetValue(start.ss);

        if (!loaded.end_time.empty()) {
            TimeUtils::ParsedTime end = TimeUtils::ParseUTCString(loaded.end_time);
            m_cb_end->SetValue(true);
            m_dp_end->SetValue(end.date);
            m_tc_end_hhmm->SetValue(end.hhmm);
            m_tc_end_ss->SetValue(end.ss);
        }

        // 割り切れる最大の単位で表示する
        if (loaded.target_time > 0 && loaded.target_time % 3600 == 0) {
            m_ch_target_unit->SetSelection(UNIT_HOURS);
            m_sc_target_value->SetValue(loaded.target_time / 3600.0);
        } else if (loaded.target_time > 0 && loaded.target_time % 60 == 0) {
            m_ch_target_unit->SetSelection(UNIT_MINUTES);
            m_sc_target_value->SetValue(loaded.target_time / 60.0);
        } else {
            m_ch_target_unit->SetSelection(UNIT_SECONDS);
            m_sc_target_value->SetValue(static_cast<double>(loaded.target_time));
        }

        m_tc_memo->SetValue(wxString::FromUTF8(loaded.memo));
    }

    UpdateSelectedName();
    UpdatePeriodControls();
    UpdateEndControls();
    UpdateTargetLimit();

    // イベント
    m_btn_end_now->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxDateTime now = wxDateTime::Now();
        m_dp_end->SetValue(now);
        m_tc_end_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_end_ss->SetValue(now.Format("%S"));
        UpdateTargetLimit();
        UpdateTargetWarning();
    });
    // 日付を変えたときも、周期未定義の上限を更新する
    m_dp_start->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) { UpdateTargetLimit(); UpdateTargetWarning(); });
    m_dp_end->Bind(wxEVT_DATE_CHANGED,   [this](wxDateEvent&) { UpdateTargetLimit(); UpdateTargetWarning(); });
    btn_select->Bind(wxEVT_BUTTON, &EditGoalDlg::OnSelect, this);
    btn_save->Bind(wxEVT_BUTTON, &EditGoalDlg::OnSave, this);
    btn_cancel->Bind(wxEVT_BUTTON, &EditGoalDlg::OnCancel, this);

    btn_now->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxDateTime now = wxDateTime::Now();
        m_dp_start->SetValue(now);
        m_tc_start_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_start_ss->SetValue(now.Format("%S"));
        UpdateTargetLimit();
        UpdateTargetWarning();
    });

    // 種別を切り替えたら、その種別で保持している選択を表示に反映する
    m_ch_target_kind->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateSelectedName(); });
    m_ch_period_type->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdatePeriodControls(); });
    // 最大稼働時間の表示は入力に追従させ、赤字の警告は入力が確定したときだけ更新する
    m_sc_every_n->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { UpdateTargetLimit(); UpdateTargetWarning(); });
    m_sc_every_n->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { UpdateTargetLimit(); });
    m_sc_every_n->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { UpdateTargetWarning(); e.Skip(); });
    m_sc_target_value->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { UpdateTargetWarning(); });
    m_sc_target_value->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { UpdateTargetWarning(); e.Skip(); });
    m_ch_target_unit->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateTargetLimit(); UpdateTargetWarning(); });

    // 時刻入力変換
    m_tc_start_hhmm->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnValidateHHMM(m_tc_start_hhmm); });
    m_tc_start_hhmm->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        OnValidateHHMM(m_tc_start_hhmm);
        e.Skip();
    });
    m_tc_start_ss->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnValidateSS(m_tc_start_ss); });
    m_tc_start_ss->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        OnValidateSS(m_tc_start_ss);
        e.Skip();
    });

    m_cb_end->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdateEndControls(); });
    m_tc_end_hhmm->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnValidateHHMM(m_tc_end_hhmm); });
    m_tc_end_hhmm->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        if (m_cb_end->GetValue()) OnValidateHHMM(m_tc_end_hhmm); // 無効時は検証しない
        e.Skip();
    });
    m_tc_end_ss->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { OnValidateSS(m_tc_end_ss); });
    m_tc_end_ss->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
        if (m_cb_end->GetValue()) OnValidateSS(m_tc_end_ss);
        e.Skip();
    });
}

void EditGoalDlg::UpdateEndControls() {
    const bool enabled = m_cb_end->GetValue();
    m_dp_end->Enable(enabled);
    m_tc_end_hhmm->Enable(enabled);
    m_tc_end_ss->Enable(enabled);
    m_btn_end_now->Enable(enabled);
    UpdateTargetLimit(); // 周期未定義のときは End の有無で最大稼働時間が変わる
    UpdateTargetWarning();
}

void EditGoalDlg::OnSave(wxCommandEvent& WXUNUSED(event)) {
    // 選択中の種別の ID だけが有効 (category_id / todo_id はどちらか一方のみ保存する)
    const bool is_category = m_ch_target_kind->GetSelection() == KIND_CATEGORY;
    if ((is_category ? m_category_id : m_todo_id) == -1) {
        wxMessageBox(_("Please select a category or todo"), "Error", wxOK | wxICON_WARNING);
        return;
    }
    if (m_tc_start_hhmm->GetValue().IsEmpty() || m_tc_start_ss->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter valid time"), "Error", wxOK | wxICON_WARNING);
        return;
    }
    if (GetTargetSeconds() <= 0) {
        wxMessageBox(_("Please enter a target time"), "Error", wxOK | wxICON_WARNING);
        return;
    }

    // 表示はシステム(ローカル)時間、保存は UTC
    const std::string start_time = TimeUtils::BuildUTCString(m_dp_start->GetValue(), m_tc_start_hhmm->GetValue(), m_tc_start_ss->GetValue());
    std::string end_time; // 空 = 期限なし
    if (m_cb_end->GetValue()) {
        if (m_tc_end_hhmm->GetValue().IsEmpty() || m_tc_end_ss->GetValue().IsEmpty()) {
            wxMessageBox(_("Please enter valid time"), "Error", wxOK | wxICON_WARNING);
            return;
        }
        end_time = TimeUtils::BuildUTCString(m_dp_end->GetValue(), m_tc_end_hhmm->GetValue(), m_tc_end_ss->GetValue());
        // "YYYY-MM-DD HH:MM:SS" 形式なので文字列比較で前後を判定できる
        if (end_time <= start_time) {
            wxMessageBox(_("End time must be after start time"), "Error", wxOK | wxICON_WARNING);
            return;
        }
    }

    // 期間の長さ(周期未定義のときは Start〜End)を超える目標は登録できない。上限なし(-1)ならチェックしない
    // 周期未定義の上限は End に依存するので、End の検証のあとで判定する
    const long long period = GetPeriodSeconds();
    if (period >= 0 && GetTargetSeconds() > period) {
        UpdateTargetWarning(); // 赤字の警告も最新にしたうえで、ポップアップでも知らせる
        wxMessageBox(_("Target time exceeds the length of the period"), "Error", wxOK | wxICON_WARNING);
        return;
    }

    Database::Goal goal;
    // 選択中の種別の方だけを設定し、もう一方は -1 (NULL) のままにする
    goal.category_id = is_category ? m_category_id : -1;
    goal.todo_id     = is_category ? -1 : m_todo_id;
    goal.goal_name   = m_tc_name->GetValue().utf8_string();
    goal.period_type = m_ch_period_type->GetSelection();
    goal.period_n    = (goal.period_type == PERIOD_EVERY_N_DAYS) ? m_sc_every_n->GetValue() : -1;
    goal.target_time = GetTargetSeconds();
    goal.start_time  = start_time;
    goal.end_time    = end_time;
    goal.is_active   = m_cb_active->GetValue() ? 1 : 0;
    goal.memo        = m_tc_memo->GetValue().utf8_string();

    bool saved;
    if (m_goal_id != -1) {
        goal.goal_id = m_goal_id;
        saved = m_db.UpdateGoal(goal);
    } else {
        saved = m_db.InsertGoal(goal);
    }
    if (!saved) {
        wxMessageBox(_("Failed to save"), "Error", wxOK | wxICON_ERROR);
        return;
    }
    EndModal(wxID_OK);
}

void EditGoalDlg::OnCancel(wxCommandEvent& WXUNUSED(event)) {
    EndModal(wxID_CANCEL);
}

void EditGoalDlg::OnSelect(wxCommandEvent& WXUNUSED(event)) {
    if (m_ch_target_kind->GetSelection() == KIND_CATEGORY) {
        SelCategoryDlg dlg(this, m_db, true); // Goal はフォルダにも設定できる
        if (dlg.ShowModal() == wxID_OK) {
            m_category_id = dlg.GetSelectedCategoryId();
        }
    } else {
        SelToDoDlg dlg(this, m_db);
        if (dlg.ShowModal() == wxID_OK) {
            m_todo_id = dlg.GetSelectedTodoId();
        }
    }
    UpdateSelectedName();
}

void EditGoalDlg::UpdateSelectedName() {
    // 現在選択中の種別のメンバ変数だけを参照する (もう一方は保持したまま)
    const bool is_category = m_ch_target_kind->GetSelection() == KIND_CATEGORY;
    if (is_category && m_category_id != -1) {
        m_st_selected_name->SetLabel(wxString::FromUTF8(m_db.GetCategoryName(m_category_id)));
        m_st_selected_path->SetLabel(wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id)));
        m_st_selected_id->SetLabel(wxString::Format("%d", m_category_id));
    } else if (!is_category && m_todo_id != -1) {
        m_st_selected_name->SetLabel(wxString::FromUTF8(m_db.GetTodoById(m_todo_id).todo_name));
        m_st_selected_path->SetLabel("-"); // ToDo にはパスがない
        m_st_selected_id->SetLabel(wxString::Format("%d", m_todo_id));
    } else {
        m_st_selected_name->SetLabel("-");
        m_st_selected_path->SetLabel("-");
        m_st_selected_id->SetLabel("-");
    }
    Layout();
}

void EditGoalDlg::UpdatePeriodControls() {
    const bool every_n = m_ch_period_type->GetSelection() == PERIOD_EVERY_N_DAYS;
    m_sc_every_n->Enable(every_n);
    m_st_days->Enable(every_n);
    UpdateTargetLimit(); // 期間が変わると最大稼働時間も変わる
    UpdateTargetWarning();
}

long long EditGoalDlg::GetTargetSeconds() const {
    const double value = m_sc_target_value->GetValue();
    switch (m_ch_target_unit->GetSelection()) {
    // 浮動小数点の誤差で 1 秒ずれないよう、切り捨てではなく四捨五入する
    case UNIT_MINUTES: return std::llround(value * 60);
    case UNIT_HOURS:   return std::llround(value * 3600);
    default:           return std::llround(value);
    }
}

long long EditGoalDlg::GetPeriodSeconds() const {
    const long long day = 24 * 3600;
    switch (m_ch_period_type->GetSelection()) {
    case PERIOD_WEEKLY:       return 7 * day;
    case PERIOD_MONTHLY:      return 31 * day; // 最大の月に合わせる(有効な目標を弾かないため)
    case PERIOD_EVERY_N_DAYS: return m_sc_every_n->GetValue() * day;
    case PERIOD_UNDEFINED: {
        // 周期未定義: End があれば Start との差分が最大稼働時間。End がなければ上限なし(-1)
        if (!m_cb_end->GetValue()) return -1;
        if (m_tc_start_hhmm->GetValue().IsEmpty() || m_tc_start_ss->GetValue().IsEmpty() ||
            m_tc_end_hhmm->GetValue().IsEmpty()   || m_tc_end_ss->GetValue().IsEmpty()) return -1;
        const long long start = TimeUtils::ParseUTCStringToEpoch(
            TimeUtils::BuildUTCString(m_dp_start->GetValue(), m_tc_start_hhmm->GetValue(), m_tc_start_ss->GetValue()));
        const long long end = TimeUtils::ParseUTCStringToEpoch(
            TimeUtils::BuildUTCString(m_dp_end->GetValue(), m_tc_end_hhmm->GetValue(), m_tc_end_ss->GetValue()));
        return end > start ? end - start : 0; // End が Start 以前なら、何も登録できない
    }
    default:                  return day;      // DAILY
    }
}

void EditGoalDlg::UpdateTargetLimit() {
    // 最大稼働時間 (期間の長さ) を、選択中の単位で表示する
    double divisor = 1;
    switch (m_ch_target_unit->GetSelection()) {
    case UNIT_MINUTES: divisor = 60;   break;
    case UNIT_HOURS:   divisor = 3600; break;
    default: break;
    }
    const long long period = GetPeriodSeconds();
    if (period < 0) {
        m_st_target_max->SetLabel("/ -"); // 上限なし
    } else {
        // seconds は整数、minutes / hour は小数第 2 位まで表示する (周期未定義だと端数が出るため)
        const int digits = (divisor == 1) ? 0 : 2;
        m_st_target_max->SetLabel(wxString::Format("/ %.*f", digits, period / divisor));
    }
    Layout();
}

void EditGoalDlg::UpdateTargetWarning() {
    const long long period = GetPeriodSeconds();
    const bool over = period >= 0 && GetTargetSeconds() > period;
    m_st_target_warn->SetLabel(over ? _("Target time exceeds the maximum working time of the period") : wxString());
    Layout();
}

void EditGoalDlg::OnValidateHHMM(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseHHMM(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid time"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
    UpdateTargetLimit(); // Start / End が変わると周期未定義の上限も変わる
    UpdateTargetWarning();
}

void EditGoalDlg::OnValidateSS(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseSS(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
    UpdateTargetLimit();
    UpdateTargetWarning();
}
