#include "todo.hpp"
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/wx.h>
#include <wx/datetime.h>

ToDo::ToDo(wxWindow* parent, Database &dbRef) 
	: wxFrame(parent, wxID_ANY, wxT("ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , m_db(dbRef) {

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1000, 600))); 

    wxPanel* main_panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* dvlc_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* st_choiceCondition = new wxStaticText(main_panel, wxID_ANY, _("Condition: "));

	m_choiceCondition = new wxChoice(main_panel, wxID_ANY);
	m_choiceCondition->Append(_("All"));
    m_choiceCondition->Append(_("Pending"));
	m_choiceCondition->Append(_("Done"));
	m_choiceCondition->SetSelection(0); // 初期表示は All

    m_btn_add = new wxButton(main_panel, wxID_ANY, _("Add"));
    m_btn_edit = new wxButton(main_panel, wxID_ANY, _("Edit"));

    ctrl_sizer->Add(st_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(m_btn_add, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_btn_edit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

    sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));
    
    m_dvlc = new wxDataViewListCtrl(main_panel, wxID_ANY);

    // 列の定義
    m_dvlc->AppendTextColumn(_("Todo ID"), wxDATAVIEW_CELL_INERT, FromDIP(75));
    m_dvlc->AppendTextColumn(_("Status"), wxDATAVIEW_CELL_INERT, FromDIP(50));
    m_dvlc->AppendTextColumn(_("Priority"), wxDATAVIEW_CELL_INERT, FromDIP(50));
    m_dvlc->AppendTextColumn(_("Todo Name"), wxDATAVIEW_CELL_INERT, FromDIP(125));
    m_dvlc->AppendTextColumn(_("Start Time"), wxDATAVIEW_CELL_INERT, FromDIP(125));
    m_dvlc->AppendTextColumn(_("Deadline"), wxDATAVIEW_CELL_INERT, FromDIP(125));
    m_dvlc->AppendTextColumn(_("Remaining Days"), wxDATAVIEW_CELL_INERT, FromDIP(100));
    m_dvlc->AppendTextColumn(_("Completion Date"), wxDATAVIEW_CELL_INERT, FromDIP(125));
    m_dvlc->AppendTextColumn(_("Memo"), wxDATAVIEW_CELL_INERT, FromDIP(100));

    // m_dvlc->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &Recording::OnSelectionChanged, this); // 行選択イベント

    sizer->Add(m_dvlc, 1, wxEXPAND | wxALL, FromDIP(10));
    main_panel->SetSizer(sizer);
    CenterOnParent();

    m_choiceCondition->Bind(wxEVT_CHOICE, &ToDo::OnConditionChanged, this);

    RefreshList();
}

void ToDo::OnConditionChanged(wxCommandEvent& WXUNUSED(event)) {
    RefreshList();
}

void ToDo::RefreshList() {
    Database::ToDoFilter filter;
    switch (m_choiceCondition->GetSelection()) {
    case 1:  filter = Database::ToDoFilter::Pending; break;
    case 2:  filter = Database::ToDoFilter::Done;    break;
    default: filter = Database::ToDoFilter::All;     break;
    }

    std::vector<Database::ToDo> todo_list = m_db.GetTodoList(filter);
    wxDateTime today = wxDateTime::Today();

    m_dvlc->DeleteAllItems();
    for (const Database::ToDo& todo : todo_list) {
        wxString remaining_days = "-";
        wxDateTime deadline;
        if (!todo.deadline.empty() && deadline.ParseISODate(wxString::FromUTF8(todo.deadline))) {
            wxTimeSpan diff = deadline - today;
            remaining_days = wxString::Format("%d", diff.GetDays());
        }

        wxVector<wxVariant> row;
        row.push_back(wxVariant(wxString::Format("%d", todo.todo_id)));
        row.push_back(wxVariant(todo.status == 1 ? _("Done") : _("Pending")));
        row.push_back(wxVariant(wxString::Format("%d", todo.priority)));
        row.push_back(wxVariant(wxString::FromUTF8(todo.todo_name)));
        row.push_back(wxVariant(wxString::FromUTF8(todo.time_begin)));
        row.push_back(wxVariant(wxString::FromUTF8(todo.deadline)));
        row.push_back(wxVariant(remaining_days));
        row.push_back(wxVariant(wxString::FromUTF8(todo.completion_date)));
        row.push_back(wxVariant(wxString::FromUTF8(todo.memo)));
        m_dvlc->AppendItem(row);
    }
}


