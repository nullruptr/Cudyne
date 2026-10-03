#include "edit_goal_dlg.hpp"
#include "core/utils/format_time.hpp"
#include "gui/common/sel_category_dlg/sel_category_dlg.hpp"
#include "gui/common/sel_todo_dlg/sel_todo_dlg.hpp"
#include <cmath>
#include <wx/sizer.h>
#include <wx/wx.h>

namespace {
enum TargetKind { KIND_CATEGORY = 0, KIND_TODO = 1 };
enum PeriodType { PERIOD_DAILY = 0, PERIOD_WEEKLY, PERIOD_MONTHLY, PERIOD_EVERY_N_DAYS };
enum TargetUnit { UNIT_SECONDS = 0, UNIT_MINUTES, UNIT_HOURS };
}

EditGoalDlg::EditGoalDlg(wxWindow* parent, Database& dbRef, int category_id, int todo_id)
    : wxDialog(parent, wxID_ANY, _("Goal"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) {

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
    grid->Add(new wxStaticText(this, wxID_ANY, _("Name")), labelFlags);
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

    // Target time
    m_sc_target_value = new wxSpinCtrlDouble(this, wxID_ANY, "60", wxDefaultPosition, FromDIP(wxSize(90, -1)), wxSP_ARROW_KEYS, 0, 1000000, 60, 1);
    m_sc_target_value->SetDigits(2); // 2.5 minutes などの小数入力を許可する
    wxArrayString units;
    units.Add(_("seconds"));
    units.Add(_("minutes"));
    units.Add(_("hour"));
    m_ch_target_unit = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, units);
    m_ch_target_unit->SetSelection(UNIT_MINUTES);
    m_st_target_sec = new wxStaticText(this, wxID_ANY, "");
    wxBoxSizer* target_sizer = new wxBoxSizer(wxHORIZONTAL);
    target_sizer->Add(m_sc_target_value, 0, wxALIGN_CENTER_VERTICAL);
    target_sizer->Add(m_ch_target_unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    target_sizer->Add(m_st_target_sec, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Target time")), labelFlags);
    grid->Add(target_sizer, labelFlags);

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

    UpdateSelectedName();
    UpdatePeriodControls();
    UpdateTargetSeconds();

    // イベント
    btn_select->Bind(wxEVT_BUTTON, &EditGoalDlg::OnSelect, this);
    btn_save->Bind(wxEVT_BUTTON, &EditGoalDlg::OnSave, this);
    btn_cancel->Bind(wxEVT_BUTTON, &EditGoalDlg::OnCancel, this);

    btn_now->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxDateTime now = wxDateTime::Now();
        m_dp_start->SetValue(now);
        m_tc_start_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_start_ss->SetValue(now.Format("%S"));
    });

    // 種別を切り替えたら、その種別で保持している選択を表示に反映する
    m_ch_target_kind->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateSelectedName(); });
    m_ch_period_type->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdatePeriodControls(); });
    m_sc_target_value->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { UpdateTargetSeconds(); });
    m_sc_target_value->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { UpdateTargetSeconds(); });
    m_ch_target_unit->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdateTargetSeconds(); });

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
}

void EditGoalDlg::OnSave(wxCommandEvent& WXUNUSED(event)) {
    if (m_tc_name->GetValue().IsEmpty()) {
        wxMessageBox(_("Please enter a goal name"), "Error", wxOK | wxICON_WARNING);
        return;
    }
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

    // TODO: goal テーブルへの保存は Database に Goal の API を追加してから実装する
    //   category_id / todo_id : 選択中の種別の方だけ (is_category ? m_category_id : m_todo_id)、もう一方は NULL
    //   goal_name             : m_tc_name
    //   period_type / period_n: m_ch_period_type / m_sc_every_n
    //   target_time           : GetTargetSeconds()
    //   start_date            : TimeUtils::BuildUTCString(m_dp_start, hhmm, ss)
    //   is_active / memo      : m_cb_active / m_tc_memo
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

void EditGoalDlg::UpdateTargetSeconds() {
    m_st_target_sec->SetLabel(wxString::Format("= %lld sec", GetTargetSeconds()));
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
}

void EditGoalDlg::OnValidateSS(wxTextCtrl* tc) {
    wxString result = TimeUtils::ParseSS(tc->GetValue());
    if (result.IsEmpty()) {
        wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING);
        tc->SetValue("");
    } else {
        tc->SetValue(result);
    }
}
