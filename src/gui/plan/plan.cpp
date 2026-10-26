#include <wx/gdicmn.h>
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/dateevt.h>
#include <wx/splitter.h>
#include <wx/wrapsizer.h>
#include <algorithm>
#include <ctime>
#include <functional>
#include <set>
#include "plan.hpp"
#include "plan_add_dlg.hpp"
#include "plan_check_panel.hpp"
#include "core/plan/plan_conflict.hpp"
#include "gui/common/wheel_choice.hpp"
#include "core/utils/format_time.hpp"
#include "gui/common/sel_category_dlg/sel_category_dlg.hpp"

namespace {
const char* const kTimeFormat = "%Y-%m-%d %H:%M:%S";

// 表に入力された時刻 (ローカル) を読む。"YYYY-MM-DD HH:MM:SS" か、秒なしの "YYYY-MM-DD HH:MM"
bool ParseLocalTime(const wxString& text, wxDateTime& out) {
    wxString s = text;
    s.Trim().Trim(false);
    for (const char* format : {"%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M"}) {
        wxDateTime dt;
        wxString::const_iterator end;
        if (dt.ParseFormat(s, format, &end) && end == s.end() && dt.IsValid()) {
            out = dt;
            return true;
        }
    }
    return false;
}

// ローカル時刻 -> DB に保存する UTC 文字列
std::string ToUtcString(const wxDateTime& local) {
    return local.ToUTC().Format(kTimeFormat).ToStdString();
}

// "YYYY-MM-DD HH:MM:SS" の先頭の日付 ("YYYY-MM-DD")。読めなければ空
std::string DateKey(const wxString& time_text) {
    const std::string s = time_text.ToStdString();
    return s.size() >= 10 ? s.substr(0, 10) : std::string();
}

// 時刻を、n (負なら過去) 単位ぶんずらす。日・週・月は暦で動かす (時間の増減ではなく、夏時間でも同じ時刻になる)
void ShiftTime(wxDateTime& dt, int n, int unit) {
    switch (unit) {
    case 0: dt += wxTimeSpan::Minutes(n); break;
    case 1: dt += wxTimeSpan::Hours(n);   break;
    case 2: dt += wxDateSpan::Days(n);    break;
    case 3: dt += wxDateSpan::Weeks(n);   break;
    default: dt += wxDateSpan::Months(n); break;
    }
}

const wxColour kColourNew(226, 239, 218);       // 新規: 薄い緑
const wxColour kColourModified(255, 242, 204);  // 変更: 薄い黄
const wxColour kColourReadOnly(240, 240, 240);  // 編集できないセル: 薄い灰
const wxColour kColourConflict(255, 199, 206);  // 同じカテゴリで時間が重なる行: 赤
const wxColour kColourEmptyDay(217, 217, 217);  // 予定のない日の行: 灰
const wxColour kColourEmptyDayText(120, 120, 120);
const wxColour kColourCountdown(221, 235, 247); // 次の予定までのカウントダウンの行: 薄い青
}

Plan::Plan(wxWindow* parent, Database &dbRef)
	: wxPanel(parent, wxID_ANY)
      , m_db(dbRef)
      , m_check_timer(this)
      , m_countdown_timer(this) {

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(5);

    // --- 上段: 絞り込みと操作 ---
    // 横長にならないよう、縦に並べた行のサイザーに分ける
    wxBoxSizer* toolbar = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* row_filter = new wxBoxSizer(wxHORIZONTAL); // Filter by period ~ Clear
    wxBoxSizer* row_edit = new wxBoxSizer(wxHORIZONTAL);   // Show empty days ~ Copy memo
    wxBoxSizer* row_save = new wxBoxSizer(wxHORIZONTAL);   // Save 以降
    wxBoxSizer* row_time = new wxBoxSizer(wxHORIZONTAL);   // 選択中の start_time / end_time セルの編集

    // 絞り込み (Detail の「Filter by period」と同じ)
    const long date_picker_style = wxDP_DROPDOWN | wxDP_SHOWCENTURY;
    m_cb_period = new wxCheckBox(this, wxID_ANY, _("Filter by period"));
    wxDateTime first_day = wxDateTime::Today();
    first_day.SetDay(1);
    wxDateTime last_day = first_day;
    last_day.SetToLastMonthDay();
    m_dp_from = new wxDatePickerCtrl(this, wxID_ANY, first_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_dp_to   = new wxDatePickerCtrl(this, wxID_ANY, last_day, wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_cb_period->SetValue(true); // 予定が溜まると表が大きくなるので、既定は今月だけ
    wxButton* btn_category = new wxButton(this, wxID_ANY, _("Category..."));
    m_st_category = new wxStaticText(this, wxID_ANY, _("All categories"));
    wxButton* btn_clear_category = new wxButton(this, wxID_ANY, _("Clear"));
    m_cb_empty_days = new wxCheckBox(this, wxID_ANY, _("Show empty days"));

    row_filter->Add(m_cb_period, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    row_filter->Add(m_dp_from, 0, wxALIGN_CENTER_VERTICAL);
    row_filter->Add(new wxStaticText(this, wxID_ANY, wxT("~")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, gap);
    row_filter->Add(m_dp_to, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(15));
    row_filter->Add(btn_category, 0, wxALIGN_CENTER_VERTICAL);
    row_filter->Add(m_st_category, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, gap);
    row_filter->Add(btn_clear_category, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(15));
    row_edit->Add(m_cb_empty_days, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(25));

    // 行の操作
    wxButton* btn_add = new wxButton(this, wxID_ANY, _("Add Row"));
    wxButton* btn_delete = new wxButton(this, wxID_ANY, _("Delete Row"));
    row_edit->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    row_edit->Add(btn_delete, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(15));

    // 複製。右側の設定は、元の予定から何(単位)ずらして複製するか。メモを複製するかも選べる
    wxButton* btn_duplicate = new wxButton(this, wxID_ANY, _("Duplicate"));
    m_tc_shift = new wxTextCtrl(this, wxID_ANY, "1", wxDefaultPosition, FromDIP(wxSize(60, -1)));
    wxArrayString units;
    units.Add(_("minutes"));
    units.Add(_("hours"));
    units.Add(_("days"));
    units.Add(_("weeks"));
    units.Add(_("months"));
    m_ch_unit = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, units);
    m_ch_unit->SetSelection(UNIT_DAYS);
    WheelChoice::Bind(m_ch_unit); // マウスホイールで、単位を切り替えられる
    m_cb_copy_memo = new wxCheckBox(this, wxID_ANY, _("Copy memo"));
    m_cb_copy_memo->SetValue(true);
    row_edit->Add(btn_duplicate, 0, wxALIGN_CENTER_VERTICAL);
    row_edit->Add(new wxStaticText(this, wxID_ANY, _("Shift by")), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_edit->Add(m_tc_shift, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_edit->Add(m_ch_unit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_edit->Add(m_cb_copy_memo, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(10));
    row_edit->AddSpacer(FromDIP(15));

    // 保存
    wxButton* btn_save = new wxButton(this, wxID_ANY, _("Save"));
    wxButton* btn_discard = new wxButton(this, wxID_ANY, _("Discard"));
    m_st_status = new wxStaticText(this, wxID_ANY, "");
    row_save->Add(btn_save, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    row_save->Add(btn_discard, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(15));
    row_save->Add(m_st_status, 0, wxALIGN_CENTER_VERTICAL);

    // 4 行目: 選択中の start_time / end_time セルを、日付 (カレンダー)・時刻・秒で編集する。入力は Edit ToDo と同じ
    m_st_time_target = new wxStaticText(this, wxID_ANY, "");
    m_dp_time = new wxDatePickerCtrl(this, wxID_ANY, wxDateTime::Now(), wxDefaultPosition, wxDefaultSize, date_picker_style);
    m_tc_hhmm = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(50, -1)), wxTE_PROCESS_ENTER);
    m_tc_ss = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, FromDIP(wxSize(35, -1)), wxTE_PROCESS_ENTER);
    m_btn_time_now = new wxButton(this, wxID_ANY, _("= Now"));
    row_time->Add(m_dp_time, 0, wxALIGN_CENTER_VERTICAL);
    row_time->Add(m_tc_hhmm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_time->Add(m_tc_ss, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_time->Add(m_btn_time_now, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row_time->Add(m_st_time_target, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(15));

    toolbar->Add(row_filter, 0, wxEXPAND | wxBOTTOM, gap);
    toolbar->Add(row_edit, 0, wxEXPAND | wxBOTTOM, gap);
    toolbar->Add(row_save, 0, wxEXPAND | wxBOTTOM, gap);
    toolbar->Add(row_time, 0, wxEXPAND);
    sizer->Add(toolbar, 0, wxEXPAND | wxALL, FromDIP(8));

    // --- 表と、下のペイン ---
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                      wxSP_LIVE_UPDATE | wxSP_3DSASH);
    m_grid = new wxGrid(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_check_panel = new PlanCheckPanel(splitter);

    // 列のラベルを付ける前に、表を作る (作る前に付けるとクラッシュする)
    m_grid->CreateGrid(0, COL_COUNT);
    m_grid->SetColLabelValue(COL_ID, "ID");
    m_grid->SetColLabelValue(COL_CATEGORY, "Category ID");
    m_grid->SetColLabelValue(COL_PATH, "Category Path");
    m_grid->SetColLabelValue(COL_START, "start_time");
    m_grid->SetColLabelValue(COL_END, "end_time");
    m_grid->SetColLabelValue(COL_TOTAL, "Total");
    m_grid->SetColLabelValue(COL_COUNTDOWN, "Countdown");
    m_grid->SetColLabelValue(COL_MEMO, "memo");

    m_grid->SetColSize(COL_ID, FromDIP(50));
    m_grid->SetColSize(COL_CATEGORY, FromDIP(90));
    m_grid->SetColSize(COL_PATH, FromDIP(200));
    m_grid->SetColSize(COL_START, FromDIP(150));
    m_grid->SetColSize(COL_END, FromDIP(150));
    m_grid->SetColSize(COL_TOTAL, FromDIP(80));
    m_grid->SetColSize(COL_COUNTDOWN, FromDIP(100));
    m_grid->SetColSize(COL_MEMO, FromDIP(200));
    m_grid->SetRowLabelSize(FromDIP(100)); // 行ラベルに、開始日 (YYYY-MM-DD) を出す

    // ID、パス、合計時間は、他のセルから決まる値なので編集できない
    for (int col : {static_cast<int>(COL_ID), static_cast<int>(COL_PATH), static_cast<int>(COL_TOTAL), static_cast<int>(COL_COUNTDOWN)}) {
        wxGridCellAttr* attr = new wxGridCellAttr; // 所有権は表に移る
        attr->SetReadOnly();
        m_grid->SetColAttr(col, attr);
    }

    splitter->SetMinimumPaneSize(FromDIP(120)); // どちらも、ドラッグで消えないようにする
    splitter->SetSashGravity(1.0);              // ウィンドウの高さを変えたら、上 (表) だけが伸縮する。下は固定
    splitter->SplitHorizontally(m_grid, m_check_panel, -FromDIP(260)); // 負の値 = 下のペインの高さ。上に表、下に Goal の達成判定

    sizer->Add(splitter, 1, wxEXPAND);
    SetSizer(sizer);

    // --- イベント ---
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { RunCheck(); }, m_check_timer.GetId());
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { UpdateCountdown(); }, m_countdown_timer.GetId());
    m_countdown_timer.Start(1000);
    FindNextStart();
    UpdateCountdown();
    m_cb_period->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnFilterChanged(); });
    // From > To にならないよう、動かした側に合わせてもう一方を寄せる
    m_dp_from->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
        if (m_dp_from->GetValue() > m_dp_to->GetValue()) m_dp_to->SetValue(m_dp_from->GetValue());
        OnFilterChanged();
    });
    m_dp_to->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) {
        if (m_dp_to->GetValue() < m_dp_from->GetValue()) m_dp_from->SetValue(m_dp_to->GetValue());
        OnFilterChanged();
    });
    btn_category->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChooseCategoryFilter(); });
    btn_clear_category->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_category_filter == -1) return;
        m_category_filter = -1;
        m_st_category->SetLabel(_("All categories"));
        OnFilterChanged();
    });
    m_cb_empty_days->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        CaptureGrid(); // 編集中の内容を取り込んでから、描き直す (DB は読み直さない)
        Render();
    });
    btn_add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { AddRow(); });
    btn_delete->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DeleteSelectedRows(); });
    btn_duplicate->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { DuplicateSelected(); });
    btn_save->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Save(); });
    btn_discard->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Discard(); });
    m_grid->Bind(wxEVT_GRID_CELL_CHANGING, &Plan::OnCellChanging, this);
    m_grid->Bind(wxEVT_GRID_CELL_CHANGED, &Plan::OnCellChanged, this);
    m_grid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK, &Plan::OnCellDoubleClick, this);
    m_grid->Bind(wxEVT_GRID_SELECT_CELL, &Plan::OnSelectCell, this);
    // 行ラベルをクリックして行を選んだとき、キーボードのフォーカスを表に移す。
    // 移さないと、直前に触った入力欄にフォーカスが残り、Ctrl+C / Ctrl+V や矢印キーがそちらに届いてしまう
    m_grid->Bind(wxEVT_GRID_LABEL_LEFT_CLICK, [this](wxGridEvent& event) {
        event.Skip();
        CallAfter([this]() { m_grid->GetGridWindow()->SetFocus(); });
    });

    // 日付はカレンダーで選ぶと、すぐ反映する。時刻と秒は、Enter かフォーカスが外れたときに確定する (Edit ToDo と同じ)
    m_dp_time->Bind(wxEVT_DATE_CHANGED, [this](wxDateEvent&) { ApplyTimeFromControls(); });
    for (wxTextCtrl* tc : {m_tc_hhmm, m_tc_ss}) {
        tc->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { ApplyTimeFromControls(); });
        tc->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
            ApplyTimeFromControls();
            e.Skip();
        });
    }
    m_btn_time_now->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (m_time_row < 0) return;
        const wxDateTime now = wxDateTime::Now();
        m_dp_time->SetValue(now);
        m_tc_hhmm->ChangeValue(now.Format("%H:%M"));
        m_tc_ss->ChangeValue(now.Format("%S"));
        ApplyTimeFromControls();
    });
    m_grid->Bind(wxEVT_KEY_DOWN, &Plan::OnKeyDown, this);
    // フォーカスは表の内側のウィンドウにあるので、そちらでも受ける (処理したらイベントは止まるので、二重には動かない)
    m_grid->GetGridWindow()->Bind(wxEVT_KEY_DOWN, &Plan::OnKeyDown, this);

    UpdateFilterControls();
    LoadFromDb();
    SyncTimeEditor(-1, -1); // 最初は、どのセルも選ばれていない
}

bool Plan::HasUnsavedChanges() const {
    if (!m_deleted_ids.empty()) return true;
    return std::any_of(m_rows.begin(), m_rows.end(), [](const PlanRow& r) { return r.state != RowState::Clean; });
}

bool Plan::ConfirmDiscard() {
    if (!HasUnsavedChanges()) return true;
    return wxMessageBox(_("Discard unsaved changes?"), _("Confirm"),
                        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) == wxYES;
}

// --- 絞り込み ---

Plan::FilterState Plan::CurrentFilter() const {
    FilterState state;
    state.period = m_cb_period->GetValue();
    state.from = m_dp_from->GetValue();
    state.to = m_dp_to->GetValue();
    state.category_id = m_category_filter;
    return state;
}

void Plan::ApplyFilterToControls(const FilterState& state) {
    m_cb_period->SetValue(state.period);
    m_dp_from->SetValue(state.from);
    m_dp_to->SetValue(state.to);
    m_category_filter = state.category_id;
    m_st_category->SetLabel(state.category_id == -1
        ? wxString(_("All categories"))
        : wxString::FromUTF8(m_db.GetCategoriesPath(state.category_id)));
    UpdateFilterControls();
    Layout();
}

void Plan::UpdateFilterControls() {
    m_dp_from->Enable(m_cb_period->GetValue());
    m_dp_to->Enable(m_cb_period->GetValue());
}

// 絞り込みを変えると DB から読み直すので、未保存の変更があれば確認する。取り消したら、元の絞り込みに戻す
void Plan::OnFilterChanged() {
    UpdateFilterControls();
    if (!ConfirmDiscard()) {
        ApplyFilterToControls(m_applied);
        return;
    }
    LoadFromDb();
}

void Plan::ChooseCategoryFilter() {
    SelCategoryDlg dlg(this, m_db, true); // フォルダを選ぶと、配下のタスクの予定がまとめて出る
    if (dlg.ShowModal() != wxID_OK) return;

    const int id = dlg.GetSelectedCategoryId();
    if (id == -1) return;

    m_category_filter = id;
    m_st_category->SetLabel(wxString::FromUTF8(m_db.GetCategoriesPath(id)));
    Layout();
    OnFilterChanged();
}

// --- 読み込みと描画 ---

void Plan::LoadFromDb() {
    m_applied = CurrentFilter();
    m_rows.clear();
    m_deleted_ids.clear();

    // 期間で絞るときは、[From の 0 時, To の翌日 0 時) と重なる予定。絞らないときはすべて
    std::string from_utc = "0000-01-01 00:00:00";
    std::string to_utc;
    if (m_applied.period) {
        wxDateTime from = m_applied.from;
        from.ResetTime();
        wxDateTime to = m_applied.to;
        to.ResetTime();
        to.Add(wxDateSpan::Day());
        from_utc = ToUtcString(from);
        to_utc = ToUtcString(to);
    }

    for (const Database::PlanItem& item : m_db.GetPlans(from_utc, to_utc, m_applied.category_id)) {
        PlanRow row;
        row.id = item.id;
        row.goal_id = item.goal_id;
        row.cell[COL_CATEGORY] = wxString::Format("%d", item.category_id);
        row.cell[COL_START] = TimeUtils::FormatUTCStringToLocal(item.start_time);
        row.cell[COL_END] = TimeUtils::FormatUTCStringToLocal(item.end_time);
        row.cell[COL_MEMO] = wxString::FromUTF8(item.memo);
        m_rows.push_back(row);
    }
    Render();
}

// 表で編集した内容を m_rows に取り込む。行を増減して描き直す前に呼ぶ
void Plan::CaptureGrid() {
    m_grid->SaveEditControlValue(); // 編集中のセルを確定する

    for (int g = 0; g < static_cast<int>(m_grid_to_data.size()); ++g) {
        const int i = m_grid_to_data[g];
        if (i < 0 || i >= static_cast<int>(m_rows.size())) continue;
        for (int col : {static_cast<int>(COL_CATEGORY), static_cast<int>(COL_START), static_cast<int>(COL_END), static_cast<int>(COL_MEMO)}) {
            m_rows[i].cell[col] = m_grid->GetCellValue(g, col);
        }
    }
}

// パスは Category ID から、合計時間は start / end から求める。ID も、ここで文字列にそろえる
void Plan::ComputeDerived(Database& db, PlanRow& row) {
    row.cell[COL_ID] = row.id == -1 ? wxString("-") : wxString::Format("%d", row.id);

    long category_id = 0;
    row.cell[COL_PATH].Clear();
    if (row.cell[COL_CATEGORY].ToLong(&category_id) && category_id > 0) {
        row.cell[COL_PATH] = wxString::FromUTF8(db.GetCategoriesPath(static_cast<int>(category_id)));
    }

    wxDateTime start, end;
    row.cell[COL_TOTAL] = "-";
    if (ParseLocalTime(row.cell[COL_START], start) && ParseLocalTime(row.cell[COL_END], end) && end > start) {
        row.cell[COL_TOTAL] = TimeUtils::FormatSeconds(static_cast<long long>(end.GetTicks()) - static_cast<long long>(start.GetTicks()));
    }
}

// m_rows から表を描き直す。「Show empty days」が ON のときは、予定のない日の灰色の行を差し込む
void Plan::Render() {
    m_grid->BeginBatch();
    if (m_grid->GetNumberRows() > 0) m_grid->DeleteRows(0, m_grid->GetNumberRows());
    m_grid_to_data.clear();
    m_data_to_grid.assign(m_rows.size(), -1);

    for (PlanRow& row : m_rows) ComputeDerived(m_db, row);
    RecomputeConflicts();

    // 予定のない日 (昇順)。範囲は、期間で絞っているときはその期間、そうでなければ最初〜最後の予定の日
    std::vector<std::string> empty_days;
    if (m_cb_empty_days->GetValue()) {
        std::set<std::string> have;
        for (const PlanRow& row : m_rows) {
            const std::string key = DateKey(row.cell[COL_START]);
            if (!key.empty()) have.insert(key);
        }

        std::string first, last;
        if (m_applied.period) {
            first = m_applied.from.Format("%Y-%m-%d").ToStdString();
            last = m_applied.to.Format("%Y-%m-%d").ToStdString();
        } else if (!have.empty()) {
            first = *have.begin();
            last = *have.rbegin();
        }

        wxDateTime day;
        constexpr int kMaxEmptyDays = 2000; // 範囲が極端に長くても、表が膨らみすぎないように
        if (!first.empty() && ParseLocalTime(first + " 00:00:00", day)) {
            for (int n = 0; n < kMaxEmptyDays; ++n, day += wxDateSpan::Day()) {
                const std::string key = day.Format("%Y-%m-%d").ToStdString();
                if (key > last) break;
                if (have.count(key) == 0) empty_days.push_back(key);
            }
        }
    }

    // 予定の行と、空き日の行を、日付の順に並べる
    struct Entry { int data_index; int empty_index; };
    std::vector<Entry> sequence;
    size_t next_empty = 0;
    // カウントダウンの行 (empty_index = -2) は、現在時刻より後に始まる最初の予定の直前に入れる。なければ最後
    const long long now = static_cast<long long>(std::time(nullptr));
    bool countdown_placed = false;
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        const std::string key = DateKey(m_rows[i].cell[COL_START]);
        while (!key.empty() && next_empty < empty_days.size() && empty_days[next_empty] < key) {
            sequence.push_back({-1, static_cast<int>(next_empty++)});
        }
        wxDateTime start;
        if (!countdown_placed && ParseLocalTime(m_rows[i].cell[COL_START], start) && static_cast<long long>(start.GetTicks()) > now) {
            sequence.push_back({-1, -2});
            countdown_placed = true;
        }
        sequence.push_back({i, 0});
    }
    while (next_empty < empty_days.size()) sequence.push_back({-1, static_cast<int>(next_empty++)});
    if (!countdown_placed) sequence.push_back({-1, -2});

    if (!sequence.empty()) m_grid->AppendRows(static_cast<int>(sequence.size()));
    m_countdown_row = -1;
    for (int g = 0; g < static_cast<int>(sequence.size()); ++g) {
        if (sequence[g].data_index < 0 && sequence[g].empty_index == -2) {
            m_grid_to_data.push_back(-1);
            m_countdown_row = g;
            RenderCountdownRow(g);
        } else if (sequence[g].data_index >= 0) {
            m_grid_to_data.push_back(sequence[g].data_index);
            m_data_to_grid[sequence[g].data_index] = g;
            RenderRow(g, sequence[g].data_index);
        } else {
            m_grid_to_data.push_back(-1);
            RenderEmptyDayRow(g, wxString::FromUTF8(empty_days[sequence[g].empty_index]));
        }
    }

    // 複製・追加した行を選択する。カーソルは、最初の行の入力すべきセルに置く
    bool cursor_set = false;
    m_grid->ClearSelection();
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        if (!m_rows[i].select_after_render) continue;
        m_rows[i].select_after_render = false;
        const int g = m_data_to_grid[i];
        m_grid->SelectRow(g, true);
        if (!cursor_set) {
            m_grid->SetGridCursor(g, m_rows[i].cell[COL_CATEGORY].IsEmpty() ? COL_CATEGORY : COL_START);
            m_grid->MakeCellVisible(g, COL_START);
            cursor_set = true;
        }
    }

    m_grid->EndBatch();
    UpdateStatus();
    SyncTimeEditor(m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol()); // 描き直したので、入力欄も合わせる
    ScheduleCheck(); // 予定が変わったので、Goal の達成判定もやり直す
}

// --- Goal の達成判定 (下のペイン) ---

void Plan::ScheduleCheck() {
    m_check_panel->SetBusy();
    m_check_timer.StartOnce(400); // 続けて編集している間は、待ち直す
}

// 予定が変わるたびに呼ばれる ScheduleCheck の後 (RunCheck) で、次の開始も探し直す
void Plan::FindNextStart() {
    const long long now = static_cast<long long>(std::time(nullptr));
    m_next_start = -1;
    for (const PlanFeasibility::PlanSpan& span : CollectEffectivePlans(now, now + 366LL * 24 * 3600)) {
        if (span.start > now && (m_next_start == -1 || span.start < m_next_start)) m_next_start = span.start;
    }
}

// 予定の行の Countdown 列: 開始まで残り (HH:MM:SS) / 進行中 / 終了
wxString Plan::RowCountdownText(const PlanRow& row, long long now) const {
    wxDateTime start, end;
    if (!ParseLocalTime(row.cell[COL_START], start)) return "-";
    const long long s = static_cast<long long>(start.GetTicks());
    if (s > now) return TimeUtils::FormatSeconds(s - now);
    if (ParseLocalTime(row.cell[COL_END], end) && static_cast<long long>(end.GetTicks()) > now) return _("In progress");
    return _("Ended");
}

void Plan::UpdateRowCountdowns() {
    const long long now = static_cast<long long>(std::time(nullptr));
    for (size_t i = 0; i < m_rows.size() && i < m_data_to_grid.size(); ++i) {
        const int g = m_data_to_grid[i];
        if (g < 0 || g >= m_grid->GetNumberRows()) continue;
        const wxString text = RowCountdownText(m_rows[i], now);
        if (m_grid->GetCellValue(g, COL_COUNTDOWN) != text) m_grid->SetCellValue(g, COL_COUNTDOWN, text);
    }
}

// 表のカウントダウンの行 (現在時刻の位置にある灰色でない行) の中身を、いまの時刻で書き直す
void Plan::UpdateCountdown() {
    const long long now = static_cast<long long>(std::time(nullptr));
    if (m_next_start != -1 && m_next_start <= now) {
        // 開始時刻を過ぎた: 次の予定を探し、行の位置も移す (セルを編集している最中は、次の秒に回す)
        if (m_grid->IsCellEditControlShown()) return;
        FindNextStart();
        CaptureGrid();
        Render();
        return;
    }
    UpdateRowCountdowns();
    WriteCountdownCells(now);
}

void Plan::WriteCountdownCells(long long now) {
    if (m_countdown_row < 0 || m_countdown_row >= m_grid->GetNumberRows()) return;

    for (int col = 0; col < COL_COUNT; ++col) m_grid->SetCellValue(m_countdown_row, col, "");
    if (m_next_start == -1) {
        m_grid->SetCellValue(m_countdown_row, COL_PATH, _("No upcoming plan"));
    } else {
        m_grid->SetCellValue(m_countdown_row, COL_PATH, _("Time until the next plan starts"));
        m_grid->SetCellValue(m_countdown_row, COL_START, TimeUtils::FormatSeconds(m_next_start - now));
        m_grid->SetCellValue(m_countdown_row, COL_END, wxDateTime(static_cast<time_t>(m_next_start)).Format(kTimeFormat));
    }
}

void Plan::RunCheck() {
    FindNextStart();
    UpdateCountdown();
    const long long now = static_cast<long long>(std::time(nullptr));

    // 判定する範囲: 期間で絞っているときは、その期間のうち、まだ終わっていない部分。絞っていないときは、今から 30 日
    long long from = now, to = now + 30LL * 24 * 3600;
    wxString note = _("Windows from now to 30 days ahead");
    if (m_applied.period) {
        wxDateTime f = m_applied.from;
        f.ResetTime();
        wxDateTime t = m_applied.to;
        t.ResetTime();
        t.Add(wxDateSpan::Day());
        from = std::max(now, static_cast<long long>(f.GetTicks()));
        to = static_cast<long long>(t.GetTicks());
        note = wxString::Format(_("Windows in %s - %s that have not ended"),
                                m_applied.from.Format("%Y-%m-%d"), m_applied.to.Format("%Y-%m-%d"));
    }
    // カテゴリで絞っているときは、そのカテゴリ以下の Goal だけを表示する
    if (m_applied.category_id != -1) {
        note += "\n" + wxString::Format(_("Goals of: %s"), wxString::FromUTF8(m_db.GetCategoriesPath(m_applied.category_id)));
    }
    // 選ぶ窓は、上の期間で決まる。数える予定は、窓そのものの範囲で DB から取り出し、表の未保存の変更を重ねる
    note += "\n" + wxString(_("Plans are counted over each goal's whole window: saved plans plus the unsaved changes in the table. The table's filter does not matter."));

    if (to <= from) {
        m_check_panel->SetItems({}, _("The selected period has already ended"));
        return;
    }
    m_check_panel->SetItems(
        PlanFeasibility::Check(m_db,
                               [this](long long window_from, long long window_to) { return CollectEffectivePlans(window_from, window_to); },
                               from, to, now, m_applied.category_id),
        note);
}

std::vector<PlanFeasibility::PlanSpan> Plan::CollectEffectivePlans(long long from, long long to) {
    std::vector<PlanFeasibility::PlanSpan> spans;
    std::set<int> in_grid;

    // 表の予定 (編集中・未保存の変更を含む)。カテゴリや期間で絞って見えていなくても、判定には全カテゴリの予定を使う
    for (const PlanRow& row : m_rows) {
        if (row.id != -1) in_grid.insert(row.id);

        long category_id = 0;
        wxDateTime start, end;
        if (!row.cell[COL_CATEGORY].ToLong(&category_id) || category_id <= 0) continue;
        if (!ParseLocalTime(row.cell[COL_START], start) || !ParseLocalTime(row.cell[COL_END], end) || end <= start) continue;

        PlanFeasibility::PlanSpan span;
        span.category_id = static_cast<int>(category_id);
        span.start = static_cast<long long>(start.GetTicks());
        span.end = static_cast<long long>(end.GetTicks());
        spans.push_back(span);
    }

    // 表に出ていない、保存済みの予定 (ただし、表で削除した予定は除く)
    const std::set<int> deleted(m_deleted_ids.begin(), m_deleted_ids.end());
    for (const Database::PlanItem& item : m_db.GetPlans(GoalAchieved::ToUtcString(from), GoalAchieved::ToUtcString(to))) {
        if (in_grid.count(item.id) || deleted.count(item.id)) continue;

        PlanFeasibility::PlanSpan span;
        span.category_id = item.category_id;
        span.start = TimeUtils::ParseUTCStringToEpoch(item.start_time);
        span.end = TimeUtils::ParseUTCStringToEpoch(item.end_time);
        spans.push_back(span);
    }
    return spans;
}

void Plan::RenderRow(int grid_row, int data_index) {
    const PlanRow& row = m_rows[data_index];
    for (int col = 0; col < COL_COUNT; ++col) m_grid->SetCellValue(grid_row, col, row.cell[col]);
    m_grid->SetCellValue(grid_row, COL_COUNTDOWN, RowCountdownText(row, static_cast<long long>(std::time(nullptr))));

    const std::string key = DateKey(row.cell[COL_START]);
    m_grid->SetRowLabelValue(grid_row, key.empty() ? wxString("-") : wxString::FromUTF8(key));
    UpdateRowColor(grid_row);
}

// 次の予定までのカウントダウンの行。編集できない、表示だけの行
void Plan::RenderCountdownRow(int grid_row) {
    m_grid->SetRowLabelValue(grid_row, _("Now"));
    for (int col = 0; col < COL_COUNT; ++col) {
        m_grid->SetReadOnly(grid_row, col, true);
        m_grid->SetCellBackgroundColour(grid_row, col, kColourCountdown);
        m_grid->SetCellTextColour(grid_row, col, *wxBLACK);
    }
    FindNextStart();
    WriteCountdownCells(static_cast<long long>(std::time(nullptr)));
}

// 予定のない日の行。編集できない、表示だけの行
void Plan::RenderEmptyDayRow(int grid_row, const wxString& date) {
    m_grid->SetRowLabelValue(grid_row, date);
    for (int col = 0; col < COL_COUNT; ++col) {
        m_grid->SetReadOnly(grid_row, col, true);
        m_grid->SetCellBackgroundColour(grid_row, col, kColourEmptyDay);
        m_grid->SetCellTextColour(grid_row, col, kColourEmptyDayText);
    }
}

void Plan::UpdateRowColor(int grid_row) {
    if (grid_row < 0 || grid_row >= static_cast<int>(m_grid_to_data.size())) return;
    const int i = m_grid_to_data[grid_row];
    if (i < 0) return; // 空き日の行は、色を変えない

    // 重なっている行は、新規・変更の色より優先して赤にする
    const bool conflict = i < static_cast<int>(m_conflict_partner.size()) && m_conflict_partner[i] >= 0;

    wxColour row_colour = *wxWHITE;
    switch (m_rows[i].state) {
    case RowState::New:      row_colour = kColourNew;      break;
    case RowState::Modified: row_colour = kColourModified; break;
    case RowState::Clean:    break;
    }
    if (conflict) row_colour = kColourConflict;

    for (int col = 0; col < COL_COUNT; ++col) {
        const bool read_only = col == COL_ID || col == COL_PATH || col == COL_TOTAL || col == COL_COUNTDOWN;
        m_grid->SetCellBackgroundColour(grid_row, col,
            (read_only && m_rows[i].state == RowState::Clean && !conflict) ? kColourReadOnly : row_colour);
    }
    m_grid->RefreshBlock(grid_row, 0, grid_row, COL_COUNT - 1);
}

// 同じカテゴリで、時間が重なる行を見つける。同じ時刻の行も重なり (端が接しているだけならよい)
void Plan::RecomputeConflicts() {
    m_conflict_partner.assign(m_rows.size(), -1);

    std::vector<PlanConflict::Entry> entries;
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        long category_id = 0;
        wxDateTime start, end;
        if (!m_rows[i].cell[COL_CATEGORY].ToLong(&category_id) || category_id <= 0) continue;
        if (!ParseLocalTime(m_rows[i].cell[COL_START], start) || !ParseLocalTime(m_rows[i].cell[COL_END], end)) continue;
        if (end <= start) continue;

        PlanConflict::Entry entry;
        entry.group = static_cast<int>(category_id);
        entry.start = static_cast<long long>(start.GetTicks());
        entry.end = static_cast<long long>(end.GetTicks());
        entry.index = i;
        entries.push_back(entry);
    }

    for (const std::pair<int, int>& p : PlanConflict::Find(entries)) {
        if (m_conflict_partner[p.first] < 0) m_conflict_partner[p.first] = p.second;
        if (m_conflict_partner[p.second] < 0) m_conflict_partner[p.second] = p.first;
    }
}

void Plan::RefreshAllColors() {
    for (int g = 0; g < static_cast<int>(m_grid_to_data.size()); ++g) UpdateRowColor(g);
}

void Plan::UpdateStatus(const wxString& note) {
    int modified = 0, added = 0;
    for (const PlanRow& r : m_rows) {
        if (r.state == RowState::Modified) ++modified;
        if (r.state == RowState::New) ++added;
    }
    const int deleted = static_cast<int>(m_deleted_ids.size());

    wxString text;
    if (modified + added + deleted == 0) {
        text = wxString::Format(_("%d plans"), static_cast<int>(m_rows.size()));
    } else {
        text = wxString::Format(_("Unsaved: %d new, %d modified, %d deleted"), added, modified, deleted);
    }
    const int overlapping = static_cast<int>(std::count_if(m_conflict_partner.begin(), m_conflict_partner.end(),
                                                           [](int partner) { return partner >= 0; }));
    if (overlapping > 0) text += "  " + wxString::Format(_("[%d rows overlap]"), overlapping);
    if (!note.IsEmpty()) text += "  (" + note + ")";
    m_st_status->SetLabel(text);
    Layout();
}

void Plan::MarkChanged(int data_index) {
    if (data_index < 0 || data_index >= static_cast<int>(m_rows.size())) return;
    if (m_rows[data_index].state == RowState::Clean) m_rows[data_index].state = RowState::Modified;
    UpdateRowColor(m_data_to_grid[data_index]);
    UpdateStatus();
}

// --- 行の操作 ---

// 開始時刻の順 (同じ時刻なら、あとに入れたものが下) に挿入する。入れた位置を返す
int Plan::InsertSorted(const PlanRow& row) {
    size_t pos = 0;
    while (pos < m_rows.size() && m_rows[pos].cell[COL_START] <= row.cell[COL_START]) ++pos;
    m_rows.insert(m_rows.begin() + pos, row);
    return static_cast<int>(pos);
}

// 選択中の行 (行ラベルで選択。なければカーソルのある行) の m_rows の添字を、昇順で返す
std::vector<int> Plan::SelectedDataIndices(bool include_cursor) const {
    wxArrayInt selected = m_grid->GetSelectedRows();
    if (include_cursor && selected.IsEmpty() && m_grid->GetGridCursorRow() >= 0) selected.Add(m_grid->GetGridCursorRow());

    std::vector<int> indices;
    for (int g : selected) {
        if (g < 0 || g >= static_cast<int>(m_grid_to_data.size())) continue;
        if (m_grid_to_data[g] >= 0) indices.push_back(m_grid_to_data[g]); // 空き日の行は除く
    }
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

// Add Row: 日付の組と時間帯の組を掛け合わせて、まとめて予定を追加するダイアログを開く。
// ダイアログの F3 (追加して開いたまま) で、続けて何度も追加できる
void Plan::AddRow() {
    CaptureGrid();

    // カテゴリの初期値は、カーソルのある行。なければ、いちばん下の行
    long default_category = -1;
    const int cursor_row = m_grid->GetGridCursorRow();
    if (cursor_row >= 0 && cursor_row < static_cast<int>(m_grid_to_data.size()) && m_grid_to_data[cursor_row] >= 0) {
        m_rows[m_grid_to_data[cursor_row]].cell[COL_CATEGORY].ToLong(&default_category);
    } else if (!m_rows.empty()) {
        m_rows.back().cell[COL_CATEGORY].ToLong(&default_category);
    }
    if (default_category <= 0) default_category = -1;

    PlanAddDlg dlg(this, m_db, static_cast<int>(default_category),
        // 追加を頼まれたとき: 表に新規行として足す (開始時刻の順に入る)
        [this](const PlanAddDlg::Request& request) {
            CaptureGrid();
            for (const PlanAddDlg::Entry& entry : request.entries) {
                PlanRow row;
                row.state = RowState::New;
                row.select_after_render = true;
                row.cell[COL_CATEGORY] = wxString::Format("%d", request.category_id);
                row.cell[COL_START] = entry.start.Format(kTimeFormat);
                row.cell[COL_END] = entry.end.Format(kTimeFormat);
                row.cell[COL_MEMO] = request.memo;
                InsertSorted(row);
            }
            Render();
        },
        // 重なりの目印用: 指定カテゴリの、すでにある予定。表で絞って見えていない予定や、未保存の変更も含める
        [this](int category_id) { return ExistingSpans(category_id); });
    dlg.ShowModal();
}

void Plan::DeleteSelectedRows() {
    CaptureGrid();

    std::vector<int> indices = SelectedDataIndices();
    if (indices.empty()) return;

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) { // 後ろから消す (添字がずれないように)
        // 保存済みの行は、Save するまで DB に残す。まだ保存していない行は、そのまま消える
        if (m_rows[*it].id != -1) m_deleted_ids.push_back(m_rows[*it].id);
        m_rows.erase(m_rows.begin() + *it);
    }
    Render();
}

// source の予定を、shift (単位は unit。負なら過去) ずらして複製する。新しい行は、開始時刻に合う位置に入る
void Plan::DuplicateRows(const std::vector<PlanRow>& source, int shift, int unit, bool copy_memo) {
    for (const PlanRow& original : source) {
        PlanRow row = original;
        row.id = -1;
        row.goal_id = -1; // Goal との紐づけは、複製しない
        row.state = RowState::New;
        row.select_after_render = true;

        if (shift != 0) {
            wxDateTime start, end;
            if (ParseLocalTime(row.cell[COL_START], start) && ParseLocalTime(row.cell[COL_END], end)) {
                ShiftTime(start, shift, unit);
                ShiftTime(end, shift, unit);
                row.cell[COL_START] = start.Format(kTimeFormat);
                row.cell[COL_END] = end.Format(kTimeFormat);
            }
        }
        if (!copy_memo) row.cell[COL_MEMO].Clear();

        InsertSorted(row);
    }
    Render();
}

void Plan::DuplicateSelected() {
    CaptureGrid();

    const std::vector<int> indices = SelectedDataIndices();
    if (indices.empty()) {
        wxMessageBox(_("Select rows to duplicate (click the row label)"), "Info", wxOK | wxICON_INFORMATION, this);
        return;
    }

    // ずらす量は整数 (負なら過去)。読めなければ、複製しない
    long shift = 0;
    wxString shift_text = m_tc_shift->GetValue();
    shift_text.Trim().Trim(false);
    if (!shift_text.ToLong(&shift) || shift < -100000 || shift > 100000) {
        wxMessageBox(_("Shift by must be an integer (a negative number goes back in time)"), "Error", wxOK | wxICON_WARNING, this);
        m_tc_shift->SetFocus();
        m_tc_shift->SelectAll();
        return;
    }

    std::vector<PlanRow> source;
    for (int i : indices) source.push_back(m_rows[i]);
    DuplicateRows(source, static_cast<int>(shift), m_ch_unit->GetSelection(), m_cb_copy_memo->GetValue());
}

// Ctrl+C: 選択した行の内容をためる (システムのクリップボードにも、タブ区切りの文字列を入れる)
void Plan::CopySelected() {
    CaptureGrid();

    const std::vector<int> indices = SelectedDataIndices();
    if (indices.empty()) return;

    m_clipboard.clear();
    wxString text;
    for (int i : indices) {
        m_clipboard.push_back(m_rows[i]);
        text += m_rows[i].cell[COL_CATEGORY] + "\t" + m_rows[i].cell[COL_START] + "\t" +
                m_rows[i].cell[COL_END] + "\t" + m_rows[i].cell[COL_MEMO] + "\n";
    }

    if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
    UpdateStatus(wxString::Format(_("Copied %d rows"), static_cast<int>(indices.size())));
}

// Ctrl+V: ためた行を、同じ内容で複製する (時刻はそのまま、メモも複製)
void Plan::PasteClipboard() {
    if (m_clipboard.empty()) return;
    CaptureGrid();
    DuplicateRows(m_clipboard, 0, UNIT_DAYS, true);
}

void Plan::OnKeyDown(wxKeyEvent& event) {
    // セルを編集しているときは、テキストのコピー・貼り付けに任せる
    if (event.ControlDown() && !event.AltDown() && !m_grid->IsCellEditControlShown()) {
        const int key = event.GetKeyCode();
        if (key == 'C' && !m_grid->GetSelectedRows().IsEmpty()) {
            CopySelected();
            return;
        }
        if (key == 'V' && !m_clipboard.empty()) {
            PasteClipboard();
            return;
        }
    }

    // F2: 編集画面を開く。2 行以上選んでいればまとめて、そうでなければカーソルのある行を直す。
    // (表の既定では F2 はセル内の編集を始めるが、セルを編集している最中は、そちらに任せる)
    if (event.GetKeyCode() == WXK_F2 && !event.HasAnyModifiers() && !m_grid->IsCellEditControlShown()) {
        OpenEditor();
        return;
    }
    event.Skip();
}

void Plan::SelectCategory(int grid_row) {
    if (grid_row < 0 || grid_row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[grid_row] < 0) return;

    SelCategoryDlg dlg(this, m_db, false); // 予定にできるのは、フォルダではなくタスク
    if (dlg.ShowModal() != wxID_OK) return;

    const int id = dlg.GetSelectedCategoryId();
    if (id == -1) return;

    const int i = m_grid_to_data[grid_row];
    m_grid->SetCellValue(grid_row, COL_CATEGORY, wxString::Format("%d", id));
    m_rows[i].cell[COL_CATEGORY] = wxString::Format("%d", id);
    ComputeDerived(m_db, m_rows[i]);
    m_grid->SetCellValue(grid_row, COL_PATH, m_rows[i].cell[COL_PATH]);
    RecomputeConflicts(); // カテゴリが変わると、重なる相手も変わる
    RefreshAllColors();
    MarkChanged(i);
    ScheduleCheck(); // カテゴリが変わったので、Goal の達成判定もやり直す
}

// --- セルの編集 ---

// 入力中に、おかしな値は受け付けない
void Plan::OnCellChanging(wxGridEvent& event) {
    const int row = event.GetRow();
    const int col = event.GetCol();
    const wxString value = event.GetString();

    if (row < 0 || row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[row] < 0) {
        event.Veto(); // 空き日の行は編集できない
        return;
    }

    if (col == COL_CATEGORY) {
        long id = 0;
        if (!value.ToLong(&id) || id <= 0) {
            wxMessageBox(_("Category ID must be a number"), "Error", wxOK | wxICON_WARNING, this);
            event.Veto();
        } else if (m_db.GetCategoryName(static_cast<int>(id)).empty()) {
            wxMessageBox(_("Category does not exist"), "Error", wxOK | wxICON_WARNING, this);
            event.Veto();
        } else if (m_db.IsFolder(static_cast<int>(id))) {
            wxMessageBox(_("Category is a folder (select a task)"), "Error", wxOK | wxICON_WARNING, this);
            event.Veto();
        }
    } else if (col == COL_START || col == COL_END) {
        wxDateTime dt;
        if (!ParseLocalTime(value, dt)) {
            wxMessageBox(_("Invalid time. Use YYYY-MM-DD HH:MM[:SS]"), "Error", wxOK | wxICON_WARNING, this);
            event.Veto();
        }
    }
}

void Plan::OnCellChanged(wxGridEvent& event) {
    CellValueChanged(event.GetRow(), event.GetCol());
}

// セルの値が変わったあとの更新。表への直接入力と、日時の入力欄の両方から呼ばれる
void Plan::CellValueChanged(int row, int col) {
    if (row < 0 || row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[row] < 0) return;
    const int i = m_grid_to_data[row];

    // "2026-10-21 9:00" のような入力も、"2026-10-21 09:00:00" にそろえる
    if (col == COL_START || col == COL_END) {
        wxDateTime dt;
        if (ParseLocalTime(m_grid->GetCellValue(row, col), dt)) m_grid->SetCellValue(row, col, dt.Format(kTimeFormat));
    }
    m_rows[i].cell[col] = m_grid->GetCellValue(row, col);

    ComputeDerived(m_db, m_rows[i]);
    m_grid->SetCellValue(row, COL_PATH, m_rows[i].cell[COL_PATH]);
    m_grid->SetCellValue(row, COL_TOTAL, m_rows[i].cell[COL_TOTAL]);

    // 時刻やカテゴリが変わると、重なりも変わる (相手の行の色も戻す)
    RecomputeConflicts();
    RefreshAllColors();

    // 行ラベルの日付は、開始時刻に合わせる (行の並びは、次に描き直すときに整う)
    if (col == COL_START) {
        const std::string key = DateKey(m_rows[i].cell[COL_START]);
        m_grid->SetRowLabelValue(row, key.empty() ? wxString("-") : wxString::FromUTF8(key));
    }
    MarkChanged(i);

    // 選択中のセルなら、日時の入力欄にも反映する (正規化した値になる)
    SyncTimeEditor(m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol());
    ScheduleCheck(); // 時刻やカテゴリが変わったので、Goal の達成判定もやり直す
}

// --- 選択中のセルの日時を、日付・時刻・秒で編集する入力欄 ---

void Plan::OnSelectCell(wxGridEvent& event) {
    // 時刻・秒を入力している最中に別のセルをクリックすると、フォーカスが外れる前にこちらが先に来ることがある。
    // 入力欄を別のセルの値に置き換える前に、入力中の内容を元のセルに確定する
    wxWindow* focused = wxWindow::FindFocus();
    if (focused == m_tc_hhmm || focused == m_tc_ss) ApplyTimeFromControls();

    SyncTimeEditor(event.GetRow(), event.GetCol());
    event.Skip();
}

// 選択したセルが start_time / end_time なら、その日時を入力欄に出す。それ以外なら入力欄を無効にする
void Plan::SyncTimeEditor(int row, int col) {
    m_time_row = -1;
    m_time_col = -1;

    wxDateTime dt;
    bool usable = row >= 0 && row < static_cast<int>(m_grid_to_data.size()) && m_grid_to_data[row] >= 0 &&
                  (col == COL_START || col == COL_END);
    if (usable) usable = ParseLocalTime(m_grid->GetCellValue(row, col), dt);

    m_dp_time->Enable(usable);
    m_tc_hhmm->Enable(usable);
    m_tc_ss->Enable(usable);
    m_btn_time_now->Enable(usable);

    if (usable) {
        m_time_row = row;
        m_time_col = col;
        m_dp_time->SetValue(dt);
        m_tc_hhmm->SetValue(dt.Format("%H:%M"));
        m_tc_ss->SetValue(dt.Format("%S"));
        m_st_time_target->SetLabel(wxString::Format(_("Editing: row %d, %s  (F2: edit the row in a dialog)"),
                                                    row + 1, col == COL_START ? "start_time" : "end_time"));
    } else {
        m_tc_hhmm->ChangeValue("");
        m_tc_ss->ChangeValue("");
        m_st_time_target->SetLabel(_("Select a start_time / end_time cell to edit its date and time here"));
    }
    Layout();
}

// 入力欄の日付・時刻・秒を、選択中のセルに反映する。値が変わらなければ何もしない
void Plan::ApplyTimeFromControls() {
    if (m_applying_time) return; // 確認ダイアログでフォーカスが動いて、また呼ばれたとき
    const int row = m_time_row;
    const int col = m_time_col;
    if (row < 0 || row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[row] < 0) return;

    m_applying_time = true;
    const wxString hhmm = TimeUtils::ParseHHMM(m_tc_hhmm->GetValue());
    const wxString ss = TimeUtils::ParseSS(m_tc_ss->GetValue());

    wxDateTime dt;
    if (hhmm.IsEmpty() || ss.IsEmpty() ||
        !ParseLocalTime(m_dp_time->GetValue().Format("%Y-%m-%d") + " " + hhmm + ":" + ss, dt)) {
        wxMessageBox(hhmm.IsEmpty() ? _("Invalid time") : _("Invalid seconds"), "Error", wxOK | wxICON_WARNING, this);
        SyncTimeEditor(row, col); // 入力欄を、セルの値に戻す
        m_applying_time = false;
        return;
    }

    if (dt.Format(kTimeFormat) == m_grid->GetCellValue(row, col)) {
        m_tc_hhmm->ChangeValue(hhmm); // "1300" -> "13:00" のように、入力をそろえるだけ
        m_tc_ss->ChangeValue(ss);
    } else {
        ApplyTimeValue(row, col, dt);
    }
    m_applying_time = false;
}

// 日時 (ローカル) を start_time / end_time のセルに反映する。値が変わらなければ何もしない
void Plan::ApplyTimeValue(int row, int col, const wxDateTime& dt) {
    if (row < 0 || row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[row] < 0) return;
    if (col != COL_START && col != COL_END) return;

    const wxString formatted = dt.Format(kTimeFormat);
    if (formatted == m_grid->GetCellValue(row, col)) return;

    m_grid->SetCellValue(row, col, formatted);
    CellValueChanged(row, col);
}

// 指定カテゴリの、すでにある予定の [開始, 終了) (epoch 秒)。表で絞って見えていない予定や、未保存の変更も含める
std::vector<std::pair<long long, long long>> Plan::ExistingSpans(int category_id) {
    std::vector<std::pair<long long, long long>> spans;
    const long long forever = 4102444800LL; // 2100-01-01
    for (const PlanFeasibility::PlanSpan& p : CollectEffectivePlans(0, forever)) {
        if (p.category_id == category_id) spans.emplace_back(p.start, p.end);
    }
    return spans;
}

// F2: 2 行以上選んでいれば、まとめて直す。そうでなければ、カーソルのある行を直す
void Plan::OpenEditor() {
    CaptureGrid();
    const std::vector<int> selected = SelectedDataIndices(false);
    if (selected.size() >= 2) {
        OpenRowsEditor(selected);
        return;
    }
    OpenRowEditor(m_grid->GetGridCursorRow());
}

// 選んだ複数の行を、Add Rows と同じ画面 (複数行の編集モード) で、まとめて直す。
// 値が行ごとに違う欄は空欄で出て、空欄のままなら各行の値を変えない。入力した欄だけが、全行に反映される
void Plan::OpenRowsEditor(const std::vector<int>& indices) {
    if (indices.size() < 2) return;
    CaptureGrid();

    PlanAddDlg::EditInitial initial;
    for (int index : indices) {
        if (index < 0 || index >= static_cast<int>(m_rows.size())) return;
        const PlanRow& row = m_rows[index];

        PlanAddDlg::EditInitial::Original original;
        original.row_number = m_data_to_grid[index] + 1;
        long category_id = 0;
        original.category_id = (row.cell[COL_CATEGORY].ToLong(&category_id) && category_id > 0) ? static_cast<int>(category_id) : -1;
        if (!ParseLocalTime(row.cell[COL_START], original.start) || !ParseLocalTime(row.cell[COL_END], original.end)) {
            wxMessageBox(wxString::Format(_("Row %d has an invalid time. Fix it first"), original.row_number),
                         "Error", wxOK | wxICON_WARNING, this);
            return;
        }
        original.memo = row.cell[COL_MEMO];
        initial.originals.push_back(original);
    }

    PlanAddDlg dlg(this, m_db, initial,
        // OK (F2) のとき: 選んだ行ごとに、直したあとの日時 (request.entries) を入れる。変えない欄は、その行の値のまま
        [this, indices](const PlanAddDlg::Request& request) {
            if (request.entries.size() != indices.size()) return;

            std::vector<PlanRow> updated;
            for (size_t k = 0; k < indices.size(); ++k) {
                PlanRow row = m_rows[indices[k]];
                bool changed = false;
                auto set = [&](int column, const wxString& value) {
                    if (row.cell[column] != value) {
                        row.cell[column] = value;
                        changed = true;
                    }
                };
                if (request.category_id != -1) set(COL_CATEGORY, wxString::Format("%d", request.category_id));
                set(COL_START, request.entries[k].start.Format(kTimeFormat));
                set(COL_END, request.entries[k].end.Format(kTimeFormat));
                if (request.apply_memo) set(COL_MEMO, request.memo);

                if (changed && row.state == RowState::Clean) row.state = RowState::Modified;
                row.select_after_render = true; // 直した行は、選択したままにする
                updated.push_back(row);
            }

            for (auto it = indices.rbegin(); it != indices.rend(); ++it) m_rows.erase(m_rows.begin() + *it); // 後ろから消す
            for (const PlanRow& row : updated) InsertSorted(row); // 日時が変わっても、順番どおりの位置に入る
            Render();
        },
        [this](int category_id) { return ExistingSpans(category_id); });
    dlg.ShowModal();
}

// ダブルクリック / F2: 表の grid_row の予定を、Add Rows と同じ画面 (編集モード) で直す。
// カテゴリ・日付・時間帯・メモを、まとめて直せる。直した行は、開始時刻に合う位置に移る
void Plan::OpenRowEditor(int grid_row) {
    if (grid_row < 0 || grid_row >= static_cast<int>(m_grid_to_data.size()) || m_grid_to_data[grid_row] < 0) return;

    CaptureGrid();
    const int index = m_grid_to_data[grid_row];
    const PlanRow& current = m_rows[index];

    PlanAddDlg::EditInitial initial;
    initial.row_number = grid_row + 1;
    long category_id = 0;
    initial.category_id = (current.cell[COL_CATEGORY].ToLong(&category_id) && category_id > 0) ? static_cast<int>(category_id) : -1;
    if (!ParseLocalTime(current.cell[COL_START], initial.start) || !ParseLocalTime(current.cell[COL_END], initial.end)) {
        initial.start = wxDateTime::Now();
        initial.end = initial.start + wxTimeSpan::Hours(1);
    }
    initial.memo = current.cell[COL_MEMO];

    PlanAddDlg dlg(this, m_db, initial,
        // OK (F2) のとき: この行の内容を置き換える (追加モードと同じ、1 件だけの Request で届く)
        [this, index](const PlanAddDlg::Request& request) {
            if (request.entries.size() != 1 || index < 0 || index >= static_cast<int>(m_rows.size())) return;
            const PlanAddDlg::Entry& entry = request.entries[0];

            PlanRow row = m_rows[index];
            const wxString category = wxString::Format("%d", request.category_id);
            const wxString start = entry.start.Format(kTimeFormat);
            const wxString end = entry.end.Format(kTimeFormat);
            if (row.cell[COL_CATEGORY] == category && row.cell[COL_START] == start &&
                row.cell[COL_END] == end && row.cell[COL_MEMO] == request.memo) {
                return; // 何も変わっていない
            }

            row.cell[COL_CATEGORY] = category;
            row.cell[COL_START] = start;
            row.cell[COL_END] = end;
            row.cell[COL_MEMO] = request.memo;
            if (row.state == RowState::Clean) row.state = RowState::Modified;
            row.select_after_render = true;

            m_rows.erase(m_rows.begin() + index);
            InsertSorted(row); // 開始時刻が変わっても、順番どおりの位置に入る
            Render();
        },
        [this](int category_id) { return ExistingSpans(category_id); });
    dlg.ShowModal();
}

void Plan::OnCellDoubleClick(wxGridEvent& event) {
    if (event.GetCol() == COL_CATEGORY) {
        SelectCategory(event.GetRow()); // ID を覚えていなくても、ツリーから選べる
        return;
    }
    if (event.GetCol() == COL_START || event.GetCol() == COL_END) {
        OpenRowEditor(event.GetRow()); // F2 と同じ画面を開く (空き日の行は、何もしない)
        return; // セル内の編集は始めない
    }
    event.Skip();
}

// --- 保存と破棄 ---

// row の内容を PlanItem にする。問題があれば false と、理由と、直す列
bool Plan::RowToItem(const PlanRow& row, Database::PlanItem& item, wxString& error, int& error_col) {
    item = Database::PlanItem();
    item.id = row.id;
    item.goal_id = row.goal_id;

    long category_id = 0;
    if (!row.cell[COL_CATEGORY].ToLong(&category_id) || category_id <= 0) {
        error = _("Please select a category");
        error_col = COL_CATEGORY;
        return false;
    }
    item.category_id = static_cast<int>(category_id);

    wxDateTime start, end;
    if (!ParseLocalTime(row.cell[COL_START], start)) {
        error = _("Invalid start time");
        error_col = COL_START;
        return false;
    }
    if (!ParseLocalTime(row.cell[COL_END], end)) {
        error = _("Invalid end time");
        error_col = COL_END;
        return false;
    }
    item.start_time = ToUtcString(start);
    item.end_time = ToUtcString(end);
    item.memo = row.cell[COL_MEMO].utf8_string();

    // 終了 > 開始や、カテゴリの有効性は、DB 側の検証と同じ基準で確かめる
    const std::string problem = m_db.ValidatePlanItem(item);
    if (!problem.empty()) {
        error = wxString::FromUTF8(problem);
        error_col = problem.find("start") != std::string::npos ? COL_START
                  : (problem.find("nd time") != std::string::npos ? COL_END : COL_CATEGORY);
        return false;
    }
    return true;
}

void Plan::Save() {
    CaptureGrid();

    if (!HasUnsavedChanges()) {
        wxMessageBox(_("There are no changes to save"), "Info", wxOK | wxICON_INFORMATION, this);
        return;
    }

    // 同じカテゴリで時間が重なる予定は、保存できない (同じ時刻も重なり)。
    // 変更していない行どうしの重なりは、過去に保存されたものなので、止めない
    RecomputeConflicts();
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        const int partner = m_conflict_partner[i];
        if (partner < 0 || m_rows[i].state == RowState::Clean) continue;

        wxMessageBox(wxString::Format(_("Row %d overlaps with row %d (same category and time range)"),
                                      m_data_to_grid[i] + 1, m_data_to_grid[partner] + 1),
                     "Error", wxOK | wxICON_WARNING, this);
        m_grid->SetGridCursor(m_data_to_grid[i], COL_START); // 直す場所に移る
        m_grid->MakeCellVisible(m_data_to_grid[i], COL_START);
        return;
    }

    std::vector<Database::PlanItem> created, updated;
    for (int i = 0; i < static_cast<int>(m_rows.size()); ++i) {
        if (m_rows[i].state == RowState::Clean) continue;

        Database::PlanItem item;
        wxString error;
        int error_col = COL_CATEGORY;
        if (!RowToItem(m_rows[i], item, error, error_col)) {
            wxMessageBox(wxString::Format(_("Row %d: %s"), m_data_to_grid[i] + 1, error), "Error", wxOK | wxICON_WARNING, this);
            m_grid->SetGridCursor(m_data_to_grid[i], error_col); // 直す場所に移る
            m_grid->MakeCellVisible(m_data_to_grid[i], error_col);
            return;
        }
        (m_rows[i].state == RowState::New ? created : updated).push_back(item);
    }

    // 追加・更新・削除は、まとめて 1 つのトランザクション。失敗したら何も反映されない
    std::string error;
    if (!m_db.ApplyPlanChanges(created, updated, m_deleted_ids, error)) {
        wxMessageBox(wxString::Format(_("Failed to save: %s"), wxString::FromUTF8(error)), "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    LoadFromDb(); // 新しい ID を含めて、DB の内容を読み直す
}

void Plan::Discard() {
    if (!ConfirmDiscard()) return;
    LoadFromDb();
}

PlanFrame::PlanFrame(wxWindow* parent, Database &dbRef)
	: wxFrame(parent, wxID_ANY, wxT("Plan"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1100, 800)));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_plan = new Plan(this, dbRef);
    sizer->Add(m_plan, 1, wxEXPAND);
    SetSizer(sizer);
    CenterOnParent();

    // 未保存の変更があるまま閉じるときは、確認する
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (event.CanVeto() && !m_plan->ConfirmDiscard()) {
            event.Veto();
            return;
        }
        Destroy();
    });
}
