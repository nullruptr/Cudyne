#include "todo.hpp"
#include "edit_todo_dlg.hpp"
#include "core/utils/format_time.hpp"
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/wx.h>
#include <wx/datetime.h>
#include <wx/dateevt.h>
#include <algorithm>

namespace {
    // Memo は改行が入ると行が潰れて見づらいため、一覧表示時のみ改行をスペースに置き換える
    wxString StripNewlines(const std::string& s) {
        wxString ws = wxString::FromUTF8(s);
        ws.Replace("\r\n", " ");
        ws.Replace("\n", " ");
        ws.Replace("\r", " ");
        return ws;
    }

    // 未設定(空文字)のときは "-" を表示する
    wxString CountdownOrDash(const std::string& utc_str, long long now_epoch) {
        if (utc_str.empty()) return "-";
        return TimeUtils::FormatCountdown(TimeUtils::ParseUTCStringToEpoch(utc_str), now_epoch);
    }
}

ToDo::ToDo(wxWindow* parent, Database &dbRef)
	: wxFrame(parent, wxID_ANY, wxT("ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
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
    m_choiceCondition->Append(_("Pending"));
	m_choiceCondition->Append(_("Done"));
	m_choiceCondition->SetSelection(0); // 初期表示は All

    wxStaticText* st_filter_date = new wxStaticText(main_panel, wxID_ANY, _("Show From: "));
    // wxDP_DROPDOWN でカレンダーから日付を選べるようにする。デフォルトは今日
    m_dp_filter_date = new wxDatePickerCtrl(main_panel, wxID_ANY, wxDateTime::Today(),
        wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN | wxDP_SHOWCENTURY);

    m_btn_add = new wxButton(main_panel, wxID_ANY, _("Add"));
    m_btn_edit = new wxButton(main_panel, wxID_ANY, _("Edit"));

    ctrl_sizer->Add(st_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(st_filter_date, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_dp_filter_date, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(m_btn_add, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_btn_edit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

    sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));

    m_list = new wxListCtrl(main_panel, wxID_ANY,
                             wxDefaultPosition, wxDefaultSize,
                             wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);

    // 列の定義 (クリックでソートできるよう、標準の wxListCtrl 列クリックイベントを使う)
    m_list->InsertColumn(0,  _("Todo ID"),           wxLIST_FORMAT_LEFT, FromDIP(60));
    m_list->InsertColumn(1,  _("Category ID"),       wxLIST_FORMAT_LEFT, FromDIP(70));
    m_list->InsertColumn(2,  _("Category Name"),     wxLIST_FORMAT_LEFT, FromDIP(120));
    m_list->InsertColumn(3,  _("Category Path"),     wxLIST_FORMAT_LEFT, FromDIP(160));
    m_list->InsertColumn(4,  _("Status"),            wxLIST_FORMAT_LEFT, FromDIP(70));
    m_list->InsertColumn(5,  _("Priority"),          wxLIST_FORMAT_LEFT, FromDIP(60));
    m_list->InsertColumn(6,  _("Todo Name"),         wxLIST_FORMAT_LEFT, FromDIP(140));
    m_list->InsertColumn(7,  _("Start Time"),        wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(8,  _("Time to Start"),     wxLIST_FORMAT_LEFT, FromDIP(90));
    m_list->InsertColumn(9,  _("Target End"),        wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(10, _("Time to Target End"),wxLIST_FORMAT_LEFT, FromDIP(110));
    m_list->InsertColumn(11, _("Deadline"),          wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(12, _("Time to Deadline"),  wxLIST_FORMAT_LEFT, FromDIP(100));
    m_list->InsertColumn(13, _("Completion Date"),   wxLIST_FORMAT_LEFT, FromDIP(130));
    m_list->InsertColumn(14, _("Memo"),              wxLIST_FORMAT_LEFT, FromDIP(160));

    sizer->Add(m_list, 1, wxEXPAND | wxALL, FromDIP(10));
    main_panel->SetSizer(sizer);
    CenterOnParent();

    m_choiceCondition->Bind(wxEVT_CHOICE, &ToDo::OnConditionChanged, this);
    m_dp_filter_date->Bind(wxEVT_DATE_CHANGED, &ToDo::OnFilterDateChanged, this);
    m_btn_add->Bind(wxEVT_BUTTON, &ToDo::OnAdd, this);
    m_btn_edit->Bind(wxEVT_BUTTON, &ToDo::OnEdit, this);
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ToDo::OnListDoubleClick, this);
    m_list->Bind(wxEVT_LIST_COL_CLICK, &ToDo::OnColumnClick, this);
    Bind(wxEVT_TIMER, &ToDo::OnTimer, this);

    RefreshList();
    m_timer.Start(1000); // 1秒ごとにカウントダウンを更新
}

void ToDo::OnConditionChanged(wxCommandEvent& WXUNUSED(event)) {
    RefreshList();
}

void ToDo::OnFilterDateChanged(wxDateEvent& WXUNUSED(event)) {
    RefreshList();
}

void ToDo::OnAdd(wxCommandEvent& WXUNUSED(event)) {
    EditTodoDlg* dlg = EditTodoDlg::ForNew(this, m_db);
    dlg->ShowModal();
    dlg->Destroy();
    RefreshList();
}

void ToDo::OnEdit(wxCommandEvent& WXUNUSED(event)) {
    long idx = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (idx == -1) {
        wxMessageBox(_("Please select a todo"), "Info", wxOK | wxICON_INFORMATION);
        return;
    }

    int todo_id = m_todo_cache[(size_t)idx].todo_id;
    EditTodoDlg* dlg = EditTodoDlg::ForUpdate(this, m_db, todo_id);
    dlg->ShowModal();
    dlg->Destroy();
    RefreshList();
}

void ToDo::OnListDoubleClick(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx < 0 || (size_t)idx >= m_todo_cache.size()) return;

    int todo_id = m_todo_cache[(size_t)idx].todo_id;
    EditTodoDlg* dlg = EditTodoDlg::ForUpdate(this, m_db, todo_id);
    dlg->ShowModal();
    dlg->Destroy();
    RefreshList();
}

void ToDo::OnColumnClick(wxListEvent& event) {
    int col = event.GetColumn();
    if (m_sort_column == col) {
        m_sort_ascending = !m_sort_ascending;
    } else {
        m_sort_column = col;
        m_sort_ascending = true;
    }
    ApplySort();
    RenderFromCache();
}

void ToDo::OnTimer(wxTimerEvent& WXUNUSED(event)) {
    long long now_epoch = (long long)wxDateTime::Now().GetTicks();

    for (size_t i = 0; i < m_todo_cache.size(); ++i) {
        const Database::ToDo& todo = m_todo_cache[i];
        m_list->SetItem((long)i, 8,  CountdownOrDash(todo.time_begin, now_epoch));
        m_list->SetItem((long)i, 10, CountdownOrDash(todo.target_end, now_epoch));
        m_list->SetItem((long)i, 12, CountdownOrDash(todo.deadline, now_epoch));
        m_list->SetItemBackgroundColour((long)i, ComputeRowColor(todo, now_epoch));
    }
}

wxColour ToDo::ComputeRowColor(const Database::ToDo& todo, long long now_epoch) const {
    if (todo.status == 1) return wxColour(192, 192, 192); // Done: グレー

    if (!todo.deadline.empty()) {
        long long diff = TimeUtils::ParseUTCStringToEpoch(todo.deadline) - now_epoch;
        if (diff < 0)         return wxColour(250, 205, 205); // Overdue(超過): 薄い赤
        if (diff < 24 * 3600) return wxColour(255, 243, 205); // Due soon(24時間以内): 薄い黄
    }
    return wxColour(255, 255, 255); // 通常: 白
}

void ToDo::RefreshList() {
    Database::ToDoFilter filter;
    switch (m_choiceCondition->GetSelection()) {
    case 1:  filter = Database::ToDoFilter::Pending; break;
    case 2:  filter = Database::ToDoFilter::Done;    break;
    default: filter = Database::ToDoFilter::All;     break;
    }

    std::vector<Database::ToDo> todo_list = m_db.GetTodoList(filter);

    wxDateTime today       = wxDateTime::Today();
    wxDateTime filter_date = m_dp_filter_date->GetValue();

    // DB は UTC 保存のため、日付での絞り込みもシステム(ローカル)時間に変換してから行う
    // 締切超過分は、選択中の日付に関わらず常に表示する
    m_todo_cache.clear();
    for (const Database::ToDo& todo : todo_list) {
        bool keep = true;
        if (!todo.deadline.empty()) {
            wxDateTime deadline_local = TimeUtils::ParseUTCString(todo.deadline).date;
            bool overdue = deadline_local < today;
            keep = overdue || deadline_local >= filter_date;
        }
        if (keep) m_todo_cache.push_back(todo);
    }

    ApplySort();
    RenderFromCache();
}

void ToDo::ApplySort() {
    if (m_sort_column < 0) return;

    const int col = m_sort_column;
    const bool asc = m_sort_ascending;

    auto order = [asc](auto&& av, auto&& bv) {
        return asc ? (av < bv) : (bv < av);
    };

    std::stable_sort(m_todo_cache.begin(), m_todo_cache.end(),
        [&](const Database::ToDo& a, const Database::ToDo& b) {
            switch (col) {
            case 0:  return order(a.todo_id, b.todo_id);
            case 1:  return order(a.category_id, b.category_id);
            case 4:  return order(a.status, b.status);
            case 5:  return order(a.priority, b.priority);
            case 6:  return order(a.todo_name, b.todo_name);
            case 7:
            case 8:  return order(TimeUtils::ParseUTCStringToEpoch(a.time_begin), TimeUtils::ParseUTCStringToEpoch(b.time_begin));
            case 9:
            case 10: return order(TimeUtils::ParseUTCStringToEpoch(a.target_end), TimeUtils::ParseUTCStringToEpoch(b.target_end));
            case 11:
            case 12: return order(TimeUtils::ParseUTCStringToEpoch(a.deadline), TimeUtils::ParseUTCStringToEpoch(b.deadline));
            case 13: return order(TimeUtils::ParseUTCStringToEpoch(a.completion_date), TimeUtils::ParseUTCStringToEpoch(b.completion_date));
            case 14: return order(a.memo, b.memo);
            default: return false; // Category Name/Path はキャッシュに未保持のためソート対象外
            }
        });
}

void ToDo::RenderFromCache() {
    m_list->DeleteAllItems();
    long long now_epoch = (long long)wxDateTime::Now().GetTicks();

    for (size_t i = 0; i < m_todo_cache.size(); ++i) {
        const Database::ToDo& todo = m_todo_cache[i];

        bool has_category = todo.category_id != -1;

        long idx = m_list->InsertItem((long)i, wxString::Format("%d", todo.todo_id));
        m_list->SetItem(idx, 1,  has_category ? wxString::Format("%d", todo.category_id) : "-");
        m_list->SetItem(idx, 2,  has_category ? wxString::FromUTF8(m_db.GetCategoryName(todo.category_id))  : "-");
        m_list->SetItem(idx, 3,  has_category ? wxString::FromUTF8(m_db.GetCategoriesPath(todo.category_id)) : "-");
        m_list->SetItem(idx, 4,  todo.status == 1 ? _("Done") : _("Pending"));
        m_list->SetItem(idx, 5,  wxString::Format("%d", todo.priority));
        m_list->SetItem(idx, 6,  wxString::FromUTF8(todo.todo_name));
        m_list->SetItem(idx, 7,  TimeUtils::FormatUTCStringToLocal(todo.time_begin));
        m_list->SetItem(idx, 8,  CountdownOrDash(todo.time_begin, now_epoch));
        m_list->SetItem(idx, 9,  TimeUtils::FormatUTCStringToLocal(todo.target_end));
        m_list->SetItem(idx, 10, CountdownOrDash(todo.target_end, now_epoch));
        m_list->SetItem(idx, 11, TimeUtils::FormatUTCStringToLocal(todo.deadline));
        m_list->SetItem(idx, 12, CountdownOrDash(todo.deadline, now_epoch));
        m_list->SetItem(idx, 13, TimeUtils::FormatUTCStringToLocal(todo.completion_date));
        m_list->SetItem(idx, 14, StripNewlines(todo.memo));

        m_list->SetItemBackgroundColour(idx, ComputeRowColor(todo, now_epoch));
    }
}
