#include "sel_todo_dlg.hpp"

SelToDoDlg::SelToDoDlg(wxWindow* parent, Database& dbRef)
    : wxDialog(parent, wxID_ANY, _("Select ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(dbRef) {

    SetSize(FromDIP(wxSize(500, 400)));

    m_list = new wxListCtrl(this, wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->InsertColumn(0, _("Todo ID"),   wxLIST_FORMAT_LEFT, FromDIP(60));
    m_list->InsertColumn(1, _("Todo Name"), wxLIST_FORMAT_LEFT, FromDIP(180));
    m_list->InsertColumn(2, _("Category"),  wxLIST_FORMAT_LEFT, FromDIP(120));
    m_list->InsertColumn(3, _("Status"),    wxLIST_FORMAT_LEFT, FromDIP(80));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, FromDIP(5));
    SetSizer(sizer);
    CenterOnParent();

    // 選択中の ToDo に紐付けるので、完了済みも含めて全件表示する
    m_todo_cache = m_db.GetTodoList(Database::ToDoFilter::All);
    for (size_t i = 0; i < m_todo_cache.size(); ++i) {
        const Database::ToDo& todo = m_todo_cache[i];
        bool has_category = todo.category_id != -1;

        long idx = m_list->InsertItem((long)i, wxString::Format("%d", todo.todo_id));
        m_list->SetItem(idx, 1, wxString::FromUTF8(todo.todo_name));
        m_list->SetItem(idx, 2, has_category ? wxString::FromUTF8(m_db.GetCategoryName(todo.category_id)) : "-");
        m_list->SetItem(idx, 3, todo.status == 1 ? _("Done") : _("Pending"));
    }

    // ダブルクリックで選択確定
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &SelToDoDlg::OnItemActivated, this);
}

void SelToDoDlg::OnItemActivated(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx < 0 || (size_t)idx >= m_todo_cache.size()) return;

    const Database::ToDo& todo = m_todo_cache[(size_t)idx];
    m_selected_id   = todo.todo_id;
    m_selected_name = wxString::FromUTF8(todo.todo_name);
    EndModal(wxID_OK);
}
