#include "plan_add_dlg.hpp"
#include "core/plan/plan_conflict.hpp"
#include "gui/common/sel_category_dlg/sel_category_dlg.hpp"
#include "gui/common/wheel_choice.hpp"
#include <wx/dateevt.h>

namespace {
constexpr size_t kMaxEntries = 500;   // 1 回で作る予定の上限 (日付 x 時間帯)
constexpr int kMaxRepeat = 366;       // 「繰り返し」の最大回数
const wxColour kColourOverlap(255, 199, 206);

// date の 0 時から seconds 秒後の日時。86400 以上なら翌日以降。時刻は、時・分・秒で組み立ててから日を進める
// (秒数を足すと、夏時間の切り替えがある日に、1 時間ずれるため)
wxDateTime MakeTime(const PlanInput::Date& date, int seconds) {
    const int days = seconds / 86400;
    const int rest = seconds % 86400;
    wxDateTime dt(static_cast<wxDateTime::wxDateTime_t>(date.day),
                  static_cast<wxDateTime::Month>(date.month - 1), date.year,
                  static_cast<wxDateTime::wxDateTime_t>(rest / 3600),
                  static_cast<wxDateTime::wxDateTime_t>((rest % 3600) / 60),
                  static_cast<wxDateTime::wxDateTime_t>(rest % 60));
    if (days > 0) dt += wxDateSpan::Days(days);
    return dt;
}

// 日付だけを比べた日数の差 (end が start の何日後か)。夏時間で 23 / 25 時間の日があっても、日数で数える
int DayDifference(const wxDateTime& start, const wxDateTime& end) {
    return static_cast<int>(end.GetJulianDayNumber() - start.GetJulianDayNumber() + 0.5);
}

// 開始の日の 0 時からの秒 (終了が翌日以降なら 86400 以上)
int SecondsOfDay(const wxDateTime& dt, int day_offset = 0) {
    return day_offset * 86400 + dt.GetHour() * 3600 + dt.GetMinute() * 60 + dt.GetSecond();
}

wxString TwoDigits(int value) { return wxString::Format("%02d", value); }

long long Ticks(const wxDateTime& dt) { return static_cast<long long>(dt.GetTicks()); }

bool ReadNumber(const wxTextCtrl* field, int digits, int& out) {
    return PlanInput::ParseNumber(field->GetValue().utf8_string(), digits, out);
}

// 秒は、空なら 0 (hhmm だけ打って次へ進める)。空でなくて読めないときは false
bool ReadOptionalNumber(const wxTextCtrl* field, int& out) {
    if (field->GetValue().Trim().Trim(false).IsEmpty()) {
        out = 0;
        return true;
    }
    return ReadNumber(field, 2, out);
}
}

namespace {
wxString DialogTitle(bool edit, const PlanAddDlg::EditInitial& initial) {
    if (!edit) return _("Add Rows");
    if (initial.originals.size() > 1) return wxString::Format(_("Edit %d Rows"), static_cast<int>(initial.originals.size()));
    return wxString::Format(_("Edit Row %d"), initial.row_number);
}

PlanAddDlg::EditInitial InitialForAdd(int default_category_id) {
    PlanAddDlg::EditInitial initial;
    initial.category_id = default_category_id;
    return initial;
}
}

PlanAddDlg::PlanAddDlg(wxWindow* parent, Database& dbRef, int default_category_id,
                       AddCallback on_add, SpansCallback existing_spans)
    : PlanAddDlg(parent, dbRef, false, InitialForAdd(default_category_id), std::move(on_add), std::move(existing_spans)) {
}

PlanAddDlg::PlanAddDlg(wxWindow* parent, Database& dbRef, const EditInitial& initial,
                       AddCallback on_apply, SpansCallback existing_spans)
    : PlanAddDlg(parent, dbRef, true, initial, std::move(on_apply), std::move(existing_spans)) {
}

PlanAddDlg::PlanAddDlg(wxWindow* parent, Database& dbRef, bool edit, const EditInitial& initial,
                       AddCallback on_add, SpansCallback existing_spans)
    : wxDialog(parent, wxID_ANY, DialogTitle(edit, initial),
               wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef)
    , m_on_add(std::move(on_add))
    , m_existing_spans(std::move(existing_spans))
    , m_edit(edit)
    , m_initial(initial)
    , m_multi(edit && initial.originals.size() > 1)
    , m_category_id(initial.category_id) {

    const int gap = FromDIP(8);
    const wxColour hint_colour(110, 110, 110);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* grid = new wxFlexGridSizer(0, 2, gap, gap);
    grid->AddGrowableCol(1, 1);
    const wxSizerFlags label_flags = wxSizerFlags().Align(wxALIGN_TOP);

    // --- 初期値 (追加は、今日と次の正時から 1 時間。編集は、その行の内容) ---
    wxDateTime start_dt = initial.start;
    int start_seconds = 0, end_seconds = 0;
    wxString initial_memo = m_initial.memo;

    // 複数行の編集: 全行で同じ値の入力欄だけ、初期値に出す。違う欄は空欄 (空欄のままなら、その項目は各行の値のまま)
    bool same_category = true, same_memo = true;
    bool same_year = true, same_monthday = true;
    bool same_start_hhmm = true, same_start_ss = true, same_end_hhmm = true, same_end_ss = true;
    if (m_multi) {
        const PlanAddDlg::EditInitial::Original& first = m_initial.originals.front();
        start_dt = first.start;
        start_seconds = SecondsOfDay(first.start);
        end_seconds = SecondsOfDay(first.end, DayDifference(first.start, first.end));
        initial_memo = first.memo;
        for (const PlanAddDlg::EditInitial::Original& o : m_initial.originals) {
            const int o_start = SecondsOfDay(o.start);
            const int o_end = SecondsOfDay(o.end, DayDifference(o.start, o.end));
            if (o.category_id != first.category_id) same_category = false;
            if (o.start.GetYear() != first.start.GetYear()) same_year = false;
            if (o.start.GetMonth() != first.start.GetMonth() || o.start.GetDay() != first.start.GetDay()) same_monthday = false;
            if (o_start / 60 != start_seconds / 60) same_start_hhmm = false;
            if (o_start % 60 != start_seconds % 60) same_start_ss = false;
            if (o_end / 60 != end_seconds / 60) same_end_hhmm = false;
            if (o_end % 60 != end_seconds % 60) same_end_ss = false;
            if (o.memo != first.memo) same_memo = false;
        }
        m_category_id = first.category_id;   // 先頭の行のカテゴリを出す。選び直したときだけ、全行に反映する
        m_same[COMP_YEAR] = same_year;
        m_same[COMP_MONTHDAY] = same_monthday;
        m_same[COMP_START_HHMM] = same_start_hhmm;
        m_same[COMP_START_SS] = same_start_ss;
        m_same[COMP_END_HHMM] = same_end_hhmm;
        m_same[COMP_END_SS] = same_end_ss;
        m_memo_same = same_memo;
    } else if (m_edit) {
        start_seconds = SecondsOfDay(initial.start);
        end_seconds = SecondsOfDay(initial.end, DayDifference(initial.start, initial.end));
    } else {
        start_dt = wxDateTime::Today();
        wxDateTime next_hour = wxDateTime::Now();
        start_seconds = (next_hour.GetHour() + 1) * 3600;
        if (start_seconds > PlanInput::kMaxStartHour * 3600) start_seconds = PlanInput::kMaxStartHour * 3600;
        end_seconds = start_seconds + 3600;
    }

    // --- カテゴリ ---
    wxButton* btn_category = new wxButton(this, wxID_ANY, _("Category..."));
    m_st_category = new wxStaticText(this, wxID_ANY, "");
    wxBoxSizer* category_sizer = new wxBoxSizer(wxHORIZONTAL);
    category_sizer->Add(btn_category, 0, wxALIGN_CENTER_VERTICAL);
    category_sizer->Add(m_st_category, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    grid->Add(new wxStaticText(this, wxID_ANY, _("Category")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
    grid->Add(category_sizer, wxSizerFlags(1).Expand());

    // --- 日付: 入力方法 (wxChoice) と、方法ごとの入力欄 ---
    wxBoxSizer* date_box = new wxBoxSizer(wxVERTICAL);

    wxArrayString date_modes;
    date_modes.Add(_("One day"));
    date_modes.Add(_("Several days"));
    date_modes.Add(_("Repeat"));
    m_date_mode = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, date_modes);
    m_date_mode->SetSelection(DATE_ONE);
    WheelChoice::Bind(m_date_mode); // マウスホイールで切り替えられる

    m_pick_date = new wxDatePickerCtrl(this, wxID_ANY, start_dt, wxDefaultPosition, wxDefaultSize,
                                       wxDP_DROPDOWN | wxDP_SHOWCENTURY);
    m_btn_pick = new wxButton(this, wxID_ANY, _("Set date"));

    wxBoxSizer* date_mode_row = new wxBoxSizer(wxHORIZONTAL);
    date_mode_row->Add(m_date_mode, 0, wxALIGN_CENTER_VERTICAL);
    date_mode_row->Add(m_pick_date, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);
    date_mode_row->Add(m_btn_pick, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    date_box->Add(date_mode_row, 0, wxBOTTOM, gap);

    // 1 日だけ
    m_date_one_sizer = new wxBoxSizer(wxVERTICAL);
    m_one_day = CreateDateRow(start_dt, false);
    m_date_one_sizer->Add(m_one_day.sizer, 0);
    date_box->Add(m_date_one_sizer, 0);

    // 複数の日: 行を足していく
    m_date_several_sizer = new wxBoxSizer(wxVERTICAL);
    m_date_rows_sizer = new wxBoxSizer(wxVERTICAL);
    m_date_several_sizer->Add(m_date_rows_sizer, 0);
    wxButton* btn_add_day = new wxButton(this, wxID_ANY, _("+ Add day"));
    m_date_several_sizer->Add(btn_add_day, 0, wxTOP, FromDIP(2));
    date_box->Add(m_date_several_sizer, 0);
    AddDateRow(start_dt);

    // 繰り返し: 起点の日付 + 回数 + 間隔
    m_date_repeat_sizer = new wxBoxSizer(wxVERTICAL);
    m_repeat_start = CreateDateRow(start_dt, false);
    m_date_repeat_sizer->Add(m_repeat_start.sizer, 0, wxBOTTOM, FromDIP(4));
    wxBoxSizer* repeat_row = new wxBoxSizer(wxHORIZONTAL);
    m_repeat_count = CreateField(repeat_row, _("Times"), 3, "7");
    m_repeat_step = CreateField(repeat_row, _("Every"), 3, "1");
    wxArrayString units;
    units.Add(_("days"));
    units.Add(_("weeks"));
    units.Add(_("months"));
    m_repeat_unit = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, units);
    m_repeat_unit->SetSelection(0);
    WheelChoice::Bind(m_repeat_unit);
    repeat_row->Add(m_repeat_unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(4));
    LinkFields({m_repeat_start.year, m_repeat_start.monthday, m_repeat_count, m_repeat_step}, {4, 4, 3, 3});
    m_date_repeat_sizer->Add(repeat_row, 0);
    date_box->Add(m_date_repeat_sizer, 0);

    grid->Add(new wxStaticText(this, wxID_ANY, _("Dates")), label_flags);
    grid->Add(date_box, wxSizerFlags(1).Expand());

    // --- 時間帯: 入力方法 (wxChoice) と、方法ごとの入力欄 ---
    wxBoxSizer* time_box = new wxBoxSizer(wxVERTICAL);

    wxArrayString time_modes;
    time_modes.Add(_("One time range"));
    time_modes.Add(_("Several time ranges"));
    m_time_mode = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, time_modes);
    m_time_mode->SetSelection(TIME_ONE);
    WheelChoice::Bind(m_time_mode);
    time_box->Add(m_time_mode, 0, wxBOTTOM, gap);

    m_time_one_sizer = new wxBoxSizer(wxVERTICAL);
    m_one_time = CreateTimeRow(start_seconds, end_seconds, false);
    m_time_one_sizer->Add(m_one_time.sizer, 0);
    time_box->Add(m_time_one_sizer, 0);

    m_time_several_sizer = new wxBoxSizer(wxVERTICAL);
    m_time_rows_sizer = new wxBoxSizer(wxVERTICAL);
    m_time_several_sizer->Add(m_time_rows_sizer, 0);
    wxButton* btn_add_range = new wxButton(this, wxID_ANY, _("+ Add time range"));
    m_time_several_sizer->Add(btn_add_range, 0, wxTOP, FromDIP(2));
    time_box->Add(m_time_several_sizer, 0);
    AddTimeRow(start_seconds, end_seconds);

    wxStaticText* time_hint = new wxStaticText(this, wxID_ANY,
        _("hhmm: 0930 (930 and 9 also work). The end can go past 24:00 for the next day (2600 = 2:00 the next day)"));
    time_hint->SetForegroundColour(hint_colour);
    time_box->Add(time_hint, 0, wxTOP, FromDIP(4));

    grid->Add(new wxStaticText(this, wxID_ANY, _("Times")), label_flags);
    grid->Add(time_box, wxSizerFlags(1).Expand());

    // --- メモ ---
    m_memo = new wxTextCtrl(this, wxID_ANY, m_edit ? initial_memo : wxString());
    grid->Add(new wxStaticText(this, wxID_ANY, _("Memo")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
    grid->Add(m_memo, wxSizerFlags(1).Expand());

    if (m_multi) {
        wxStaticText* multi_note = new wxStaticText(this, wxID_ANY,
            wxString::Format(_("Editing %d rows. The fields show the first row's values (yellow = the rows differ). "
                               "Only the fields you change are applied to all of them. "
                               "Click a row in the table below to edit that row only."),
                             static_cast<int>(m_initial.originals.size())));
        multi_note->SetForegroundColour(hint_colour);
        sizer->Add(multi_note, 0, wxLEFT | wxRIGHT | wxTOP, gap);

        // いま入力欄が編集している対象 (全行 / 下の一覧で選んだ行) と、全行に戻すボタン
        m_st_target = new wxStaticText(this, wxID_ANY, "");
        wxFont bold = m_st_target->GetFont();
        bold.SetWeight(wxFONTWEIGHT_BOLD);
        m_st_target->SetFont(bold);
        m_btn_all_rows = new wxButton(this, wxID_ANY, _("All rows"));
        wxBoxSizer* target_row = new wxBoxSizer(wxHORIZONTAL);
        target_row->Add(m_st_target, 0, wxALIGN_CENTER_VERTICAL);
        target_row->Add(m_btn_all_rows, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);
        sizer->Add(target_row, 0, wxLEFT | wxRIGHT | wxTOP, gap);
    }
    sizer->Add(grid, 0, wxEXPAND | wxALL, gap);

    // --- プレビュー ---
    m_st_summary = new wxStaticText(this, wxID_ANY, "");
    m_st_error = new wxStaticText(this, wxID_ANY, "");
    m_st_error->SetForegroundColour(*wxRED);
    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES);
    m_list->AppendColumn("#", wxLIST_FORMAT_RIGHT, FromDIP(40));
    m_list->AppendColumn(_("Start"), wxLIST_FORMAT_LEFT, FromDIP(190));
    m_list->AppendColumn(_("End"), wxLIST_FORMAT_LEFT, FromDIP(190));
    m_list->AppendColumn(_("Note"), wxLIST_FORMAT_LEFT, FromDIP(200));
    sizer->Add(m_st_summary, 0, wxLEFT | wxRIGHT, gap);
    sizer->Add(m_st_error, 0, wxLEFT | wxRIGHT, gap);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, gap);

    // --- ボタン ---
    m_st_status = new wxStaticText(this, wxID_ANY, "");
    wxButton* btn_ok = new wxButton(this, wxID_ANY, _("OK (F2)"));
    wxButton* btn_keep = new wxButton(this, wxID_ANY, _("Add and continue (F3)"));
    wxButton* btn_cancel = new wxButton(this, wxID_ANY, _("Close (F12)"));
    wxBoxSizer* buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(m_st_status, 1, wxALIGN_CENTER_VERTICAL);
    buttons->Add(btn_ok, 0, wxLEFT, gap);
    if (!m_edit) buttons->Add(btn_keep, 0, wxLEFT, gap); else btn_keep->Hide();
    buttons->Add(btn_cancel, 0, wxLEFT, gap);
    sizer->Add(buttons, 0, wxEXPAND | wxALL, gap);

    SetSizer(sizer);
    SetMinSize(FromDIP(wxSize(820, 640)));
    SetSize(FromDIP(wxSize(860, 720)));
    CenterOnParent();

    // --- イベント ---
    btn_category->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChooseCategory(); });
    m_btn_pick->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PickDate(); });
    btn_add_day->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        // 最後の行の翌日を、初期値にして足す (連続した日を打ちやすいように)
        wxDateTime next = m_pick_date->GetValue();
        PlanInput::Date last;
        wxString error;
        if (!m_date_rows.empty() && ReadDate(m_date_rows.back(), last, "", error)) {
            next = wxDateTime(static_cast<wxDateTime::wxDateTime_t>(last.day), static_cast<wxDateTime::Month>(last.month - 1), last.year);
            next += wxDateSpan::Day();
        }
        AddDateRow(next);
    });
    btn_add_range->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        // 最後の行の終了を、次の開始にして足す (続けて予定を並べやすいように)
        PlanInput::TimeRange last;
        wxString error;
        if (!m_time_rows.empty() && ReadTime(m_time_rows.back(), last, "", error) && last.end_seconds <= PlanInput::kMaxStartHour * 3600) {
            AddTimeRow(last.end_seconds, std::min(last.end_seconds + 3600, PlanInput::kMaxEndHour * 3600));
        } else {
            AddTimeRow(9 * 3600, 10 * 3600);
        }
    });
    m_date_mode->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ApplyDateMode(m_date_mode->GetSelection()); });
    m_time_mode->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { ApplyTimeMode(m_time_mode->GetSelection()); });
    m_repeat_unit->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { UpdatePreview(); });
    btn_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DoAdd(true); });
    btn_keep->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DoAdd(false); });
    btn_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    m_memo->Bind(wxEVT_TEXT, [this](wxCommandEvent& event) {
        if (m_memo->HasFocus()) m_touched.insert(m_memo);
        event.Skip();
    });
    if (m_multi) {
        // 下の一覧の行をクリックすると、その行だけを編集する。「All rows」で全行に戻る
        m_btn_all_rows->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { SelectTarget(-1); });
        m_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent& event) {
            if (!m_loading_list) SelectTarget(event.GetIndex());
        });
    }

    // F2 = 追加して閉じる、F3 = 追加して開いたまま、F12 = 閉じる。入力欄にフォーカスがあっても効くよう、子に渡る前に拾う
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
        switch (event.GetKeyCode()) {
        case WXK_F2:  DoAdd(true);            break;
        case WXK_F3:  if (m_edit) event.Skip(); else DoAdd(false); break;
        case WXK_F12: EndModal(wxID_CANCEL);  break;
        default:      event.Skip();           break;
        }
    });

    // 編集モードは、日付も時間帯も 1 つずつ。入力方法の選択は出さない
    if (m_edit) {
        m_date_mode->Hide();
        m_time_mode->Hide();
    }
    ApplyDateMode(DATE_ONE);
    ApplyTimeMode(TIME_ONE);

    if (m_multi) {
        // 全行向けの内容の初期値は、入力欄に出ている先頭の行の値。行ごとの内容は、まだ何も触っていない
        for (int comp = 0; comp < COMP_COUNT; ++comp) m_batch.text[comp] = ComponentField(comp)->GetValue();
        m_batch.touched = 0;
        m_row_sets.assign(m_initial.originals.size(), FieldSet());
        RefreshMixedColours();
        if (!m_memo_same) {
            m_memo->SetBackgroundColour(wxColour(255, 243, 205));
            m_memo->SetToolTip(_("The selected rows have different values here. The first row's value is shown; change it to apply your value to all rows"));
        }
        UpdateTargetLabel();
    }

    wxString category_label = m_category_id != -1 ? wxString::FromUTF8(m_db.GetCategoriesPath(m_category_id))
                                                  : wxString(_("(not selected)"));
    if (m_multi && !same_category) category_label += _("   (first row's. The rows differ; choose one to change all)");
    m_st_category->SetLabel(category_label);
    m_ready = true;
    UpdatePreview();

    m_one_day.monthday->SetFocus(); // 日と時刻から打ち始めることが多い
    m_one_day.monthday->SelectAll();
}

// --- 入力欄の作成 ---

// row に「ラベル + 入力欄」を足す。digits は桁数 (欄の幅に使う)。label が空なら、入力欄だけ
wxTextCtrl* PlanAddDlg::CreateField(wxBoxSizer* row, const wxString& label, int digits, const wxString& initial,
                                    const wxString& tooltip) {
    const int width = GetTextExtent("0").GetWidth() * digits + FromDIP(18);
    wxTextCtrl* field = new wxTextCtrl(this, wxID_ANY, initial, wxDefaultPosition, wxSize(width, -1),
                                       wxTE_CENTER);   // Enter では何も確定しない (追加は F2 / F3 のボタンだけ)
    if (!label.IsEmpty()) {
        row->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    }
    row->Add(field, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(3));
    if (!tooltip.IsEmpty()) field->SetToolTip(tooltip);

    // 打つたびに、プレビューを更新する。Skip で、同じ入力欄の他の handler (次の欄へ進める処理) も動かす
    field->Bind(wxEVT_TEXT, [this, field](wxCommandEvent& event) {
        if (field->HasFocus()) m_touched.insert(field); // 打ち込んだ欄 (プログラムが値を入れたときは、触ったことにしない)
        UpdatePreview();
        event.Skip();
    });
    return field;
}

// 並んだ入力欄で、桁がそろったら次の欄へ進む (打っている最中だけ。プログラムが値を入れたときは動かさない)
void PlanAddDlg::LinkFields(const std::vector<wxTextCtrl*>& chain, const std::vector<int>& digits) {
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
        wxTextCtrl* field = chain[i];
        wxTextCtrl* next = chain[i + 1];
        const size_t limit = static_cast<size_t>(digits[i]);
        field->Bind(wxEVT_TEXT, [this, field, next, limit](wxCommandEvent& event) {
            if (field->HasFocus() && field->GetValue().length() >= limit) {
                m_touched.insert(field); // フォーカスを動かす前に、打ち込んだ欄として覚える (複数行の編集で、全行に反映する欄)
                next->SetFocus();
                next->SelectAll();
            }
            event.Skip(); // プレビューの更新(別の handler)も続ける
        });
    }
}

PlanAddDlg::DateRow PlanAddDlg::CreateDateRow(const wxDateTime& initial, bool removable) {
    DateRow row;
    row.sizer = new wxBoxSizer(wxHORIZONTAL);
    row.year = CreateField(row.sizer, _("Select Date"), 4, wxString::Format("%04d", initial.GetYear()), "yyyy");
    row.monthday = CreateField(row.sizer, "", 4,
                               TwoDigits(static_cast<int>(initial.GetMonth()) + 1) + TwoDigits(initial.GetDay()),
                               _("mmdd (month and day, 4 digits. e.g. 1021)"));
    LinkFields({row.year, row.monthday}, {4, 4});
    if (removable) {
        row.remove = new wxButton(this, wxID_ANY, "x", wxDefaultPosition, FromDIP(wxSize(28, -1)), wxBU_EXACTFIT);
        row.sizer->Add(row.remove, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    }
    return row;
}

PlanAddDlg::TimeRow PlanAddDlg::CreateTimeRow(int start_seconds, int end_seconds, bool removable) {
    TimeRow row;
    row.sizer = new wxBoxSizer(wxHORIZONTAL);
    const wxString hhmm_tip = _("hhmm (e.g. 0930. 930 and 9 also work)");
    const wxString end_hhmm_tip = _("hhmm (e.g. 1030. Past 2400 means the next day: 2600 = 2:00 the next day)");
    row.sizer->Add(new wxStaticText(this, wxID_ANY, _("Start Time")), 0, wxALIGN_CENTER_VERTICAL);
    row.start_hhmm = CreateField(row.sizer, "", 4, TwoDigits(start_seconds / 3600) + TwoDigits((start_seconds % 3600) / 60), hhmm_tip);
    row.start_ss = CreateField(row.sizer, ":", 2, TwoDigits(start_seconds % 60), _("ss (seconds. Empty = 00)"));
    row.sizer->Add(new wxStaticText(this, wxID_ANY, _("End Time")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(18));
    row.end_hhmm = CreateField(row.sizer, "", 4, TwoDigits(end_seconds / 3600) + TwoDigits((end_seconds % 3600) / 60), end_hhmm_tip);
    row.end_ss = CreateField(row.sizer, ":", 2, TwoDigits(end_seconds % 60), _("ss (seconds. Empty = 00)"));
    LinkFields({row.start_hhmm, row.start_ss, row.end_hhmm, row.end_ss}, {4, 2, 4, 2});
    if (removable) {
        row.remove = new wxButton(this, wxID_ANY, "x", wxDefaultPosition, FromDIP(wxSize(28, -1)), wxBU_EXACTFIT);
        row.sizer->Add(row.remove, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    }
    return row;
}

// 値を入れる (ChangeValue は、入力欄の変更イベントを出さない。続けて呼び出し側が UpdatePreview する)
void PlanAddDlg::SetDateRow(const DateRow& row, const wxDateTime& dt) {
    row.year->ChangeValue(wxString::Format("%04d", dt.GetYear()));
    row.monthday->ChangeValue(TwoDigits(static_cast<int>(dt.GetMonth()) + 1) + TwoDigits(dt.GetDay()));
}

void PlanAddDlg::SetTimeRow(const TimeRow& row, int start_seconds, int end_seconds) {
    row.start_hhmm->ChangeValue(TwoDigits(start_seconds / 3600) + TwoDigits((start_seconds % 3600) / 60));
    row.start_ss->ChangeValue(TwoDigits(start_seconds % 60));
    row.end_hhmm->ChangeValue(TwoDigits(end_seconds / 3600) + TwoDigits((end_seconds % 3600) / 60));
    row.end_ss->ChangeValue(TwoDigits(end_seconds % 60));
}

// --- 入力欄 -> 値 ---

bool PlanAddDlg::ReadDate(const DateRow& row, PlanInput::Date& out, const wxString& where, wxString& error) const {
    int y = 0, m = 0, d = 0;
    if (!ReadNumber(row.year, 4, y) || !PlanInput::ParseMonthDay(row.monthday->GetValue().utf8_string(), m, d)) {
        error = where + _("Enter the year as yyyy and the month and day as 4 digits (mmdd, e.g. 1021)");
        return false;
    }
    if (!PlanInput::MakeDate(y, m, d, out)) {
        error = where + wxString::Format(_("%04d-%02d-%02d is not a valid date"), y, m, d);
        return false;
    }
    return true;
}

bool PlanAddDlg::ReadTime(const TimeRow& row, PlanInput::TimeRange& out, const wxString& where, wxString& error) const {
    int sh = 0, sm = 0, ss = 0, eh = 0, em = 0, es = 0;
    // hhmm は必須 (930 や 9 のような短い入力も可)。ss は、空なら 0
    if (!PlanInput::ParseHourMinute(row.start_hhmm->GetValue().utf8_string(), sh, sm) || !ReadOptionalNumber(row.start_ss, ss) ||
        !PlanInput::ParseHourMinute(row.end_hhmm->GetValue().utf8_string(), eh, em) || !ReadOptionalNumber(row.end_ss, es)) {
        error = where + _("Enter the time as hhmm (e.g. 0930) and the seconds as ss");
        return false;
    }
    std::string reason;
    if (!PlanInput::MakeTimeRange(sh, sm, ss, eh, em, es, out, reason)) {
        error = where + wxString::FromUTF8(reason);
        return false;
    }
    return true;
}

// いまの入力方法の、先頭の日付 (入力方法を切り替えるとき、次の方法に引き継ぐ)
bool PlanAddDlg::ReadPrimaryDate(wxDateTime& out) const {
    const DateRow* row = &m_one_day;
    if (m_date_mode_current == DATE_SEVERAL && !m_date_rows.empty()) row = &m_date_rows.front();
    if (m_date_mode_current == DATE_REPEAT) row = &m_repeat_start;

    PlanInput::Date d;
    wxString error;
    if (!ReadDate(*row, d, "", error)) return false;
    out = wxDateTime(static_cast<wxDateTime::wxDateTime_t>(d.day), static_cast<wxDateTime::Month>(d.month - 1), d.year);
    return true;
}

bool PlanAddDlg::ReadPrimaryTime(PlanInput::TimeRange& out) const {
    const TimeRow* row = &m_one_time;
    if (m_time_mode_current == TIME_SEVERAL && !m_time_rows.empty()) row = &m_time_rows.front();
    wxString error;
    return ReadTime(*row, out, "", error);
}

// --- 入力方法の切り替え ---

// 選んだ入力方法の入力欄だけを出す。先頭の日付は、前の入力方法から引き継ぐ
void PlanAddDlg::ApplyDateMode(int new_mode) {
    wxDateTime carried;
    const bool have_date = ReadPrimaryDate(carried);

    m_date_mode_current = new_mode;
    m_date_mode->SetSelection(new_mode);

    // 3 つの入力欄の組のうち、選んだものだけを出す (子の入力欄ごと)
    GetSizer()->Show(m_date_one_sizer, new_mode == DATE_ONE, true);
    GetSizer()->Show(m_date_several_sizer, new_mode == DATE_SEVERAL, true);
    GetSizer()->Show(m_date_repeat_sizer, new_mode == DATE_REPEAT, true);

    if (have_date) {
        if (new_mode == DATE_ONE) SetDateRow(m_one_day, carried);
        if (new_mode == DATE_SEVERAL && !m_date_rows.empty()) SetDateRow(m_date_rows.front(), carried);
        if (new_mode == DATE_REPEAT) SetDateRow(m_repeat_start, carried);
    }

    m_btn_pick->SetLabel(m_edit || new_mode == DATE_ONE ? _("Set date") : (new_mode == DATE_SEVERAL ? _("Add day") : _("Set start date")));
    Layout();
    UpdatePreview();
}

void PlanAddDlg::ApplyTimeMode(int new_mode) {
    PlanInput::TimeRange carried;
    const bool have_time = ReadPrimaryTime(carried);

    m_time_mode_current = new_mode;
    m_time_mode->SetSelection(new_mode);
    GetSizer()->Show(m_time_one_sizer, new_mode == TIME_ONE, true);
    GetSizer()->Show(m_time_several_sizer, new_mode == TIME_SEVERAL, true);

    if (have_time) {
        if (new_mode == TIME_ONE) SetTimeRow(m_one_time, carried.start_seconds, carried.end_seconds);
        if (new_mode == TIME_SEVERAL && !m_time_rows.empty()) SetTimeRow(m_time_rows.front(), carried.start_seconds, carried.end_seconds);
    }
    Layout();
    UpdatePreview();
}

void PlanAddDlg::AddDateRow(const wxDateTime& initial) {
    DateRow row = CreateDateRow(initial, true);
    m_date_rows.push_back(row);
    m_date_rows_sizer->Add(row.sizer, 0, wxBOTTOM, FromDIP(4));
    row.remove->Bind(wxEVT_BUTTON, [this, button = row.remove](wxCommandEvent&) { RemoveDateRow(button); });

    if (m_ready) {
        Layout();
        UpdatePreview();
        row.monthday->SetFocus();
        row.monthday->SelectAll();
    }
}

void PlanAddDlg::AddTimeRow(int start_seconds, int end_seconds) {
    TimeRow row = CreateTimeRow(start_seconds, end_seconds, true);
    m_time_rows.push_back(row);
    m_time_rows_sizer->Add(row.sizer, 0, wxBOTTOM, FromDIP(4));
    row.remove->Bind(wxEVT_BUTTON, [this, button = row.remove](wxCommandEvent&) { RemoveTimeRow(button); });

    if (m_ready) {
        Layout();
        UpdatePreview();
        row.start_hhmm->SetFocus();
        row.start_hhmm->SelectAll();
    }
}

// 行を消す。押されたボタン自身を、その場で壊さないよう、あとで消す。最後の 1 行は残す
void PlanAddDlg::RemoveDateRow(wxButton* button) {
    CallAfter([this, button]() {
        if (m_date_rows.size() <= 1) return;
        for (size_t i = 0; i < m_date_rows.size(); ++i) {
            if (m_date_rows[i].remove != button) continue;
            DateRow row = m_date_rows[i];
            m_date_rows.erase(m_date_rows.begin() + i);
            m_date_rows_sizer->Detach(row.sizer);
            row.sizer->Clear(true);
            delete row.sizer;
            Layout();
            UpdatePreview();
            return;
        }
    });
}

void PlanAddDlg::RemoveTimeRow(wxButton* button) {
    CallAfter([this, button]() {
        if (m_time_rows.size() <= 1) return;
        for (size_t i = 0; i < m_time_rows.size(); ++i) {
            if (m_time_rows[i].remove != button) continue;
            TimeRow row = m_time_rows[i];
            m_time_rows.erase(m_time_rows.begin() + i);
            m_time_rows_sizer->Detach(row.sizer);
            row.sizer->Clear(true);
            delete row.sizer;
            Layout();
            UpdatePreview();
            return;
        }
    });
}

// カレンダーで選んだ日付を、いまの入力方法に合わせて反映する
void PlanAddDlg::PickDate() {
    const wxDateTime picked = m_pick_date->GetValue();
    if (m_edit || m_date_mode_current == DATE_ONE) {
        SetDateRow(m_one_day, picked);                // 1 日: 入力欄を置き換える
        m_touched.insert(m_one_day.year);             // 複数行の編集: カレンダーで選んだ日付は、全行に反映する
        m_touched.insert(m_one_day.monthday);
    } else if (m_date_mode_current == DATE_SEVERAL) {
        AddDateRow(picked);                           // 複数の日: 行を足す
        return;
    } else {
        SetDateRow(m_repeat_start, picked);           // 繰り返し: 起点にする
    }
    UpdatePreview();
}

// --- 入力から、予定の一覧を作る ---

std::vector<PlanAddDlg::Entry> PlanAddDlg::BuildEntries(wxString& error) const {
    std::vector<Entry> entries;
    error.Clear();

    // 日付
    std::vector<PlanInput::Date> dates;
    PlanInput::Date d;
    if (m_date_mode_current == DATE_ONE) {
        if (!ReadDate(m_one_day, d, "", error)) return entries;
        dates.push_back(d);
    } else if (m_date_mode_current == DATE_SEVERAL) {
        for (size_t i = 0; i < m_date_rows.size(); ++i) {
            if (!ReadDate(m_date_rows[i], d, wxString::Format(_("Day %d: "), static_cast<int>(i) + 1), error)) return entries;
            bool seen = false;
            for (const PlanInput::Date& existing : dates) {
                if (existing.year == d.year && existing.month == d.month && existing.day == d.day) seen = true;
            }
            if (!seen) dates.push_back(d);
        }
    } else {
        int count = 0, step = 0;
        if (!ReadDate(m_repeat_start, d, _("Start: "), error)) return entries;
        if (!ReadNumber(m_repeat_count, 3, count) || count < 1 || count > kMaxRepeat ||
            !ReadNumber(m_repeat_step, 3, step) || step < 1) {
            error = wxString::Format(_("Times (1-%d) and Every (1 or more) must be numbers"), kMaxRepeat);
            return entries;
        }
        const wxDateTime base(static_cast<wxDateTime::wxDateTime_t>(d.day), static_cast<wxDateTime::Month>(d.month - 1), d.year);
        for (int i = 0; i < count; ++i) {
            // 起点から i 回ぶん進める (前の日付から足し続けると、月末の日付がずれていくため)
            wxDateTime dt = base;
            const int amount = i * step;
            switch (m_repeat_unit->GetSelection()) {
            case 1:  dt += wxDateSpan::Weeks(amount);  break;
            case 2:  dt += wxDateSpan::Months(amount); break;
            default: dt += wxDateSpan::Days(amount);   break;
            }
            dates.push_back({dt.GetYear(), static_cast<int>(dt.GetMonth()) + 1, dt.GetDay()});
        }
    }

    // 時間帯
    std::vector<PlanInput::TimeRange> times;
    PlanInput::TimeRange t;
    if (m_time_mode_current == TIME_ONE) {
        if (!ReadTime(m_one_time, t, "", error)) return entries;
        times.push_back(t);
    } else {
        for (size_t i = 0; i < m_time_rows.size(); ++i) {
            if (!ReadTime(m_time_rows[i], t, wxString::Format(_("Time range %d: "), static_cast<int>(i) + 1), error)) return entries;
            times.push_back(t);
        }
    }

    if (dates.size() * times.size() > kMaxEntries) {
        error = wxString::Format(_("Too many plans (%d x %d). The limit is %d at a time"),
                                 static_cast<int>(dates.size()), static_cast<int>(times.size()), static_cast<int>(kMaxEntries));
        return entries;
    }

    // すべての日付 x すべての時間帯
    for (const PlanInput::Date& date : dates) {
        for (const PlanInput::TimeRange& range : times) {
            Entry entry;
            entry.start = MakeTime(date, range.start_seconds);
            entry.end = MakeTime(date, range.end_seconds);
            entries.push_back(entry);
        }
    }
    return entries;
}

// 予定 index のカテゴリ。複数行の編集でカテゴリを変えないときは、その行のカテゴリ
int PlanAddDlg::CategoryOfEntry(size_t index) const {
    // 複数行の編集で、カテゴリを選び直していなければ、各行のカテゴリのまま
    if (m_multi && !m_category_touched) return index < m_initial.originals.size() ? m_initial.originals[index].category_id : -1;
    return m_category_id;
}

wxTextCtrl* PlanAddDlg::ComponentField(int component) const {
    switch (component) {
    case COMP_YEAR:       return m_one_day.year;
    case COMP_MONTHDAY:   return m_one_day.monthday;
    case COMP_START_HHMM: return m_one_time.start_hhmm;
    case COMP_START_SS:   return m_one_time.start_ss;
    case COMP_END_HHMM:   return m_one_time.end_hhmm;
    default:              return m_one_time.end_ss;
    }
}

// 入力欄の内容 (set) から、触った項目だけを値にする。where は、エラーに添える場所の名前
bool PlanAddDlg::ParseSet(const FieldSet& set, Applied& out, const wxString& where, wxString& error) const {
    for (int i = 0; i < COMP_COUNT; ++i) out.has[i] = ((set.touched >> i) & 1u) != 0;

    if (out.has[COMP_YEAR]) {
        int year = 0;
        if (!PlanInput::ParseNumber(set.text[COMP_YEAR].utf8_string(), 4, year) || year < 1970 || year > 2200) {
            error = where + _("Enter the year as yyyy");
            return false;
        }
        out.year = year;
    }
    if (out.has[COMP_MONTHDAY] && !PlanInput::ParseMonthDay(set.text[COMP_MONTHDAY].utf8_string(), out.month, out.day)) {
        error = where + _("Enter the month and day as 4 digits (mmdd, e.g. 1021)");
        return false;
    }

    auto read_hhmm = [&](int comp, int max_hour, const wxString& name, int& seconds) {
        if (!out.has[comp]) return true;
        int h = 0, m = 0;
        if (!PlanInput::ParseHourMinute(set.text[comp].utf8_string(), h, m) || PlanInput::ClockSeconds(h, m, 0, max_hour) < 0) {
            error = where + name + _("Enter the time as hhmm (e.g. 0930)");
            return false;
        }
        seconds = h * 3600 + m * 60;
        return true;
    };
    auto read_ss = [&](int comp, const wxString& name, int& seconds) {
        if (!out.has[comp]) return true;
        const std::string text = set.text[comp].utf8_string();
        if (text.find_first_not_of(" \t") == std::string::npos) { // 触って空にしたら 0 秒
            seconds = 0;
            return true;
        }
        if (!PlanInput::ParseNumber(text, 2, seconds) || seconds > 59) {
            error = where + name + _("Enter the seconds as ss (0-59)");
            return false;
        }
        return true;
    };
    return read_hhmm(COMP_START_HHMM, PlanInput::kMaxStartHour, _("Start Time: "), out.start_hhmm) &&
           read_ss(COMP_START_SS, _("Start Time: "), out.start_ss) &&
           read_hhmm(COMP_END_HHMM, PlanInput::kMaxEndHour, _("End Time: "), out.end_hhmm) &&
           read_ss(COMP_END_SS, _("End Time: "), out.end_ss);
}

// 複数行の編集: 選んだ行ごとの、直したあとの日時を作る。
//   優先順位: その行だけの内容 (一覧で選んで直したもの) > 全行向けの内容 (触った欄) > その行の元の値
//   - yyyy: 年だけ / mmdd: 月と日だけ変える (両方触ると、その日に移す)。終了が翌日以降の行は、その差を保つ
//   - 開始の hhmm / ss、終了の hhmm / ss: 触った欄だけ変える (hhmm だけ触ると、秒はそのまま)
//   - 終了の時は、開始の日からの時刻 (2600 = 翌日の 2 時)
//   - 変えたあとに、終了が開始以前になる行や、存在しない日付になる行があれば、エラー
std::vector<PlanAddDlg::Entry> PlanAddDlg::BuildMultiEntriesFrom(const FieldSet& batch, const std::vector<FieldSet>& rows,
                                                                 wxString& error) const {
    std::vector<Entry> entries;
    error.Clear();

    Applied all;
    if (!ParseSet(batch, all, "", error)) return entries;

    for (size_t i = 0; i < m_initial.originals.size(); ++i) {
        const EditInitial::Original& original = m_initial.originals[i];

        Applied own; // この行だけの内容
        if (i < rows.size() && !ParseSet(rows[i], own, wxString::Format(_("Row %d: "), original.row_number), error)) {
            entries.clear();
            return entries;
        }
        auto pick = [&](int comp) -> const Applied* { return own.has[comp] ? &own : (all.has[comp] ? &all : nullptr); };

        // 日付
        const Applied* year_from = pick(COMP_YEAR);
        const Applied* monthday_from = pick(COMP_MONTHDAY);
        const int year = year_from ? year_from->year : original.start.GetYear();
        const int month = monthday_from ? monthday_from->month : static_cast<int>(original.start.GetMonth()) + 1;
        const int day = monthday_from ? monthday_from->day : original.start.GetDay();
        PlanInput::Date row_date;
        if (!PlanInput::MakeDate(year, month, day, row_date)) {
            error = wxString::Format(_("Row %d: %04d-%02d-%02d is not a valid date"), original.row_number, year, month, day);
            entries.clear();
            return entries;
        }

        // 時刻: hhmm と ss を、それぞれ別に選ぶ
        const int original_start = SecondsOfDay(original.start);
        const int original_end = SecondsOfDay(original.end, DayDifference(original.start, original.end));
        const Applied* start_hhmm_from = pick(COMP_START_HHMM);
        const Applied* start_ss_from = pick(COMP_START_SS);
        const Applied* end_hhmm_from = pick(COMP_END_HHMM);
        const Applied* end_ss_from = pick(COMP_END_SS);
        const int start_seconds = (start_hhmm_from ? start_hhmm_from->start_hhmm : original_start / 60 * 60) +
                                  (start_ss_from ? start_ss_from->start_ss : original_start % 60);
        const int end_seconds = (end_hhmm_from ? end_hhmm_from->end_hhmm : original_end / 60 * 60) +
                                (end_ss_from ? end_ss_from->end_ss : original_end % 60);

        if (end_seconds <= start_seconds) {
            error = wxString::Format(_("Row %d: End must be after start"), original.row_number);
            entries.clear();
            return entries;
        }
        Entry entry;
        entry.start = MakeTime(row_date, start_seconds);
        entry.end = MakeTime(row_date, end_seconds);
        entries.push_back(entry);
    }
    return entries;
}

// いまの入力欄の内容を、いまの対象 (全行 / 選んだ行) に重ねて、結果を作る
std::vector<PlanAddDlg::Entry> PlanAddDlg::BuildMultiEntries(wxString& error) const {
    FieldSet batch = m_batch;
    std::vector<FieldSet> rows = m_row_sets;
    FieldSet& active = (m_active_row >= 0 && m_active_row < static_cast<int>(rows.size())) ? rows[m_active_row] : batch;

    active.touched = 0;
    for (int comp = 0; comp < COMP_COUNT; ++comp) {
        const wxTextCtrl* field = ComponentField(comp);
        active.text[comp] = field->GetValue();
        if (m_touched.count(field) > 0) active.touched |= 1u << comp;
    }
    return BuildMultiEntriesFrom(batch, rows, error);
}

// 入力欄の内容を、いまの対象に退避する (対象を切り替える前に呼ぶ)
void PlanAddDlg::StoreActive() {
    if (!m_multi) return;
    FieldSet& set = (m_active_row >= 0 && m_active_row < static_cast<int>(m_row_sets.size())) ? m_row_sets[m_active_row] : m_batch;

    set.touched = 0;
    for (int comp = 0; comp < COMP_COUNT; ++comp) {
        const wxTextCtrl* field = ComponentField(comp);
        set.text[comp] = field->GetValue();
        if (m_touched.count(field) > 0) set.touched |= 1u << comp;
    }
}

// いまの対象の内容を、入力欄に出す。全行: 全行向けの内容。一覧で選んだ行: その行の、いまの結果
void PlanAddDlg::LoadTarget() {
    FieldSet shown;
    if (m_active_row < 0) {
        shown = m_batch;
    } else {
        wxString error;
        const std::vector<Entry> entries = BuildMultiEntriesFrom(m_batch, m_row_sets, error);
        wxDateTime start = m_initial.originals[m_active_row].start;
        wxDateTime end = m_initial.originals[m_active_row].end;
        if (error.IsEmpty() && m_active_row < static_cast<int>(entries.size())) {
            start = entries[m_active_row].start;
            end = entries[m_active_row].end;
        }

        const int start_seconds = SecondsOfDay(start);
        const int end_seconds = SecondsOfDay(end, DayDifference(start, end));
        shown.text[COMP_YEAR] = wxString::Format("%04d", start.GetYear());
        shown.text[COMP_MONTHDAY] = TwoDigits(static_cast<int>(start.GetMonth()) + 1) + TwoDigits(start.GetDay());
        shown.text[COMP_START_HHMM] = TwoDigits(start_seconds / 3600) + TwoDigits((start_seconds % 3600) / 60);
        shown.text[COMP_START_SS] = TwoDigits(start_seconds % 60);
        shown.text[COMP_END_HHMM] = TwoDigits(end_seconds / 3600) + TwoDigits((end_seconds % 3600) / 60);
        shown.text[COMP_END_SS] = TwoDigits(end_seconds % 60);
        shown.touched = m_row_sets[m_active_row].touched;   // その行で触った項目は、触ったままにする
    }

    for (int comp = 0; comp < COMP_COUNT; ++comp) {
        wxTextCtrl* field = ComponentField(comp);
        field->ChangeValue(shown.text[comp]);   // 変更のイベントを出さない
        if ((shown.touched >> comp) & 1u) m_touched.insert(field); else m_touched.erase(field);
    }
    RefreshMixedColours();
}

// 入力欄の編集対象を切り替える (-1 = 全行 / 0 以上 = 下の一覧のその行だけ)
void PlanAddDlg::SelectTarget(int row) {
    if (!m_multi || row == m_active_row || row >= static_cast<int>(m_row_sets.size())) return;

    StoreActive();
    m_active_row = row;
    LoadTarget();
    UpdateTargetLabel();

    if (row < 0) { // 全行に戻したら、一覧の選択も外す
        m_loading_list = true;
        for (long i = 0; i < m_list->GetItemCount(); ++i) m_list->SetItemState(i, 0, wxLIST_STATE_SELECTED);
        m_loading_list = false;
    }
    UpdatePreview();

    m_one_day.monthday->SetFocus(); // すぐ打ち始められるように
    m_one_day.monthday->SelectAll();
}

// 値が行ごとに違う項目を、薄い黄色にする (全行を編集しているときだけ。1 行だけを編集しているときは、普通の色)
void PlanAddDlg::RefreshMixedColours() {
    const wxColour white = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour yellow(255, 243, 205);
    for (int comp = 0; comp < COMP_COUNT; ++comp) {
        wxTextCtrl* field = ComponentField(comp);
        const bool mixed = m_active_row < 0 && !m_same[comp];
        field->SetBackgroundColour(mixed ? yellow : white);
        if (mixed) {
            field->SetToolTip(_("The selected rows have different values here. The first row's value is shown; change it to apply your value to all rows"));
        } else if (m_active_row >= 0) {
            field->SetToolTip(_("Changes here apply to the selected row only"));
        }
        field->Refresh();
    }
}

void PlanAddDlg::UpdateTargetLabel() {
    if (!m_st_target) return;
    if (m_active_row < 0) {
        m_st_target->SetLabel(wxString::Format(_("Editing: all %d rows"), static_cast<int>(m_initial.originals.size())));
    } else {
        m_st_target->SetLabel(wxString::Format(_("Editing: row %d only (the other rows are not changed)"),
                                               m_initial.originals[m_active_row].row_number));
    }
    m_btn_all_rows->Enable(m_active_row >= 0);
    Layout();
}

const std::vector<std::pair<long long, long long>>& PlanAddDlg::SpansOf(int category_id) {
    auto it = m_spans_by_category.find(category_id);
    if (it == m_spans_by_category.end()) {
        it = m_spans_by_category.emplace(category_id, m_existing_spans ? m_existing_spans(category_id)
                                                                         : std::vector<std::pair<long long, long long>>()).first;
    }
    return it->second;
}

// 一覧を更新する。重なる予定 (すでにある予定と、新しく作る予定どうし) は赤にする
void PlanAddDlg::UpdatePreview() {
    if (!m_ready) return; // 画面を作っている途中

    wxString error;
    const std::vector<Entry> entries = m_multi ? BuildMultiEntries(error) : BuildEntries(error);
    m_list->DeleteAllItems();
    m_st_error->SetLabel(error);

    if (!error.IsEmpty()) {
        m_st_summary->SetLabel("");
        Layout();
        return;
    }

    // 直す前の自分自身 (編集モード) は、「すでにある予定」として数えない
    struct Own {
        int category;
        long long start;
        long long end;
    };
    std::vector<Own> own;
    if (m_multi) {
        for (const EditInitial::Original& o : m_initial.originals) own.push_back({o.category_id, Ticks(o.start), Ticks(o.end)});
    } else if (m_edit) {
        own.push_back({m_initial.category_id, Ticks(m_initial.start), Ticks(m_initial.end)});
    }

    // カテゴリごとの、すでにある予定 (自分自身を除く)
    std::map<int, std::vector<std::pair<long long, long long>>> existing_by_category;
    auto existing_of = [&](int category_id) -> const std::vector<std::pair<long long, long long>>& {
        auto found = existing_by_category.find(category_id);
        if (found != existing_by_category.end()) return found->second;

        std::vector<std::pair<long long, long long>> spans;
        if (category_id != -1) spans = SpansOf(category_id);
        for (const Own& o : own) {
            if (o.category != category_id) continue;
            for (size_t i = 0; i < spans.size(); ++i) {
                if (spans[i] == std::make_pair(o.start, o.end)) {
                    spans.erase(spans.begin() + i);
                    break;
                }
            }
        }
        return existing_by_category.emplace(category_id, std::move(spans)).first->second;
    };

    // 新しく作る予定どうしの重なり (同じカテゴリどうし)
    std::vector<PlanConflict::Entry> conflict_entries;
    for (size_t i = 0; i < entries.size(); ++i) {
        PlanConflict::Entry e;
        e.group = CategoryOfEntry(i);
        e.start = Ticks(entries[i].start);
        e.end = Ticks(entries[i].end);
        e.index = static_cast<int>(i);
        conflict_entries.push_back(e);
    }
    std::vector<bool> overlaps_new(entries.size(), false);
    for (const std::pair<int, int>& p : PlanConflict::Find(conflict_entries)) {
        overlaps_new[p.first] = true;
        overlaps_new[p.second] = true;
    }

    int overlap_count = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        const long long s = Ticks(entries[i].start);
        const long long e = Ticks(entries[i].end);
        bool overlaps_existing = false;
        for (const auto& span : existing_of(CategoryOfEntry(i))) {
            if (span.first < e && s < span.second) {
                overlaps_existing = true;
                break;
            }
        }

        wxString note;
        if (overlaps_existing) note = _("Overlaps an existing plan");
        if (overlaps_new[i]) note += (note.IsEmpty() ? "" : ", ") + wxString(m_multi ? _("Overlaps another edited row") : _("Overlaps another new plan"));

        // 左端の番号: 複数行の編集では、表の行番号
        const int number = m_multi ? m_initial.originals[i].row_number : static_cast<int>(i) + 1;
        const long row = m_list->InsertItem(m_list->GetItemCount(), wxString::Format("%d", number));
        m_list->SetItem(row, 1, entries[i].start.Format("%Y-%m-%d (%a) %H:%M:%S"));
        m_list->SetItem(row, 2, entries[i].end.Format("%Y-%m-%d (%a) %H:%M:%S"));
        m_list->SetItem(row, 3, note);
        if (!note.IsEmpty()) {
            m_list->SetItemBackgroundColour(row, kColourOverlap);
            ++overlap_count;
        }
    }

    // 一覧を作り直しても、入力欄が編集している行の選択は保つ (選択のイベントは出さない)
    if (m_multi && m_active_row >= 0 && m_active_row < m_list->GetItemCount()) {
        m_loading_list = true;
        m_list->SetItemState(m_active_row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        m_loading_list = false;
    }

    wxString summary;
    if (m_multi) {
        summary = wxString::Format(_("The %d rows will become:"), static_cast<int>(entries.size()));
    } else if (m_edit) {
        summary = _("The row will become:");
    } else {
        summary = wxString::Format(_("%d plans will be added"), static_cast<int>(entries.size()));
    }
    if (overlap_count > 0) summary += "  " + wxString::Format(_("(%d overlap: shown in red)"), overlap_count);
    m_st_summary->SetLabel(summary);
    Layout();
}

// --- カテゴリ ---

void PlanAddDlg::ChooseCategory() {
    SelCategoryDlg dlg(this, m_db, false); // 予定にできるのは、フォルダではなくタスク
    if (dlg.ShowModal() != wxID_OK) return;

    const int id = dlg.GetSelectedCategoryId();
    if (id == -1) return;

    m_category_id = id;
    m_category_touched = true;
    m_st_category->SetLabel(wxString::FromUTF8(m_db.GetCategoriesPath(id)));
    UpdatePreview();
}

// --- 追加 ---

void PlanAddDlg::SetStatus(const wxString& text) {
    m_st_status->SetLabel(text);
    Layout();
}

void PlanAddDlg::DoAdd(bool close) {
    // 複数行の編集では、カテゴリが空 (行ごとに違う) のままなら、各行のカテゴリを変えない
    if (m_category_id == -1 && !m_multi) {
        wxMessageBox(_("Please select a category"), "Error", wxOK | wxICON_WARNING, this);
        return;
    }
    if (m_category_id != -1 && (m_db.GetCategoryName(m_category_id).empty() || m_db.IsFolder(m_category_id))) {
        wxMessageBox(_("Category is not a task (select a task, not a folder)"), "Error", wxOK | wxICON_WARNING, this);
        return;
    }

    wxString error;
    Request request;
    request.entries = m_multi ? BuildMultiEntries(error) : BuildEntries(error);
    if (!error.IsEmpty()) {
        wxMessageBox(error, "Error", wxOK | wxICON_WARNING, this);
        return;
    }
    if (request.entries.empty()) {
        wxMessageBox(_("Enter at least one date and one time range"), "Error", wxOK | wxICON_WARNING, this);
        return;
    }
    // 複数行の編集では、カテゴリを選び直したときだけ全行に反映する (-1 = 各行のカテゴリのまま)
    request.category_id = (m_multi && !m_category_touched) ? -1 : m_category_id;
    request.memo = m_memo->GetValue();
    // 複数行の編集では、メモを打ち込んだときだけ全行に反映する (空にすることもできる)
    request.apply_memo = !m_multi || m_touched.count(m_memo) > 0;

    m_on_add(request);

    if (close || m_edit) {
        EndModal(wxID_OK);
        return;
    }
    // 続けて入力できるよう、開いたままにする。いま追加した予定は「すでにある予定」になるので、取り直す
    SetStatus(wxString::Format(_("Added %d plans"), static_cast<int>(request.entries.size())));
    m_spans_by_category.clear();
    ClearInputs();
    m_one_day.monthday->SetFocus();
    m_one_day.monthday->SelectAll();
}

// 追加モードの初期値 (今日、次の正時から 1 時間、メモなし) に戻す。行は 1 つずつに減らし、入力方法も 1 日 / 1 つに戻す
void PlanAddDlg::ClearInputs() {
    const wxDateTime today = wxDateTime::Today();
    int start_seconds = (wxDateTime::Now().GetHour() + 1) * 3600;
    if (start_seconds > PlanInput::kMaxStartHour * 3600) start_seconds = PlanInput::kMaxStartHour * 3600;
    const int end_seconds = start_seconds + 3600;

    while (m_date_rows.size() > 1) {
        DateRow row = m_date_rows.back();
        m_date_rows.pop_back();
        m_date_rows_sizer->Detach(row.sizer);
        row.sizer->Clear(true);
        delete row.sizer;
    }
    while (m_time_rows.size() > 1) {
        TimeRow row = m_time_rows.back();
        m_time_rows.pop_back();
        m_time_rows_sizer->Detach(row.sizer);
        row.sizer->Clear(true);
        delete row.sizer;
    }

    SetDateRow(m_one_day, today);
    SetDateRow(m_date_rows.front(), today);
    SetDateRow(m_repeat_start, today);
    m_pick_date->SetValue(today);
    m_repeat_count->ChangeValue("7");
    m_repeat_step->ChangeValue("1");
    m_repeat_unit->SetSelection(0);
    SetTimeRow(m_one_time, start_seconds, end_seconds);
    SetTimeRow(m_time_rows.front(), start_seconds, end_seconds);
    m_memo->ChangeValue("");
    m_touched.clear();

    ApplyDateMode(DATE_ONE);   // 先頭の日付を引き継ぐが、上で全部今日にしてあるので同じ
    ApplyTimeMode(TIME_ONE);
}
