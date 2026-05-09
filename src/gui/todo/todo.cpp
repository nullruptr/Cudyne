#include "todo.hpp"
#include <wx/gdicmn.h>
#include <wx/wx.h>

Todo::Todo(wxWindow* parent, Database &dbRef) 
	: wxFrame(parent, wxID_ANY, wxT("ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
	, m_db(dbRef) {

	// ウィンドウが初期化された後に FromDIP しないとクラッシュする
	SetSize(FromDIP(wxSize(800, 600))); 

	wxPanel* main_panel = new wxPanel(this, wxID_ANY);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* dvlc_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_btn_pending = new wxButton(main_panel, wxID_ANY, _("Pending"));
	m_btn_done = new wxButton(main_panel, wxID_ANY, _("Done"));
	m_btn_all = new wxButton(main_panel, wxID_ANY, _("All"));
	m_btn_add = new wxButton(main_panel, wxID_ANY, _("Add"));
	m_btn_edit = new wxButton(main_panel, wxID_ANY, _("Edit"));

	// m_btn_pending->Bind(wxEVT_BUTTON, &Recording::OnStartRecordFromBtn, this);

	ctrl_sizer->Add(m_btn_pending, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_btn_done, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_btn_all, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->AddSpacer(FromDIP(10));
	ctrl_sizer->Add(m_btn_add, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
	ctrl_sizer->Add(m_btn_edit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

	sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));
	
	m_dvlc = new wxDataViewListCtrl(main_panel, wxID_ANY);

	// 列の定義
	m_dvlc->AppendTextColumn(_("Todo ID"), wxDATAVIEW_CELL_INERT, FromDIP(75));
	m_dvlc->AppendTextColumn(_("Category Name"), wxDATAVIEW_CELL_INERT, FromDIP(150));
	m_dvlc->AppendTextColumn(_("Start Time"), wxDATAVIEW_CELL_INERT, FromDIP(175));
	m_dvlc->AppendTextColumn(_("Elapsed Time"), wxDATAVIEW_CELL_INERT, FromDIP(150));

	// m_dvlc->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &Recording::OnSelectionChanged, this); // 行選択イベント

	sizer->Add(m_dvlc, 1, wxEXPAND | wxALL, FromDIP(10));
	main_panel->SetSizer(sizer);
	CenterOnParent();
}
