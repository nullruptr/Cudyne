#include "inspector.hpp"
#include <wx/event.h>
#include <wx/filefn.h>

Inspector::Inspector(wxWindow* parent, Database &dbRef) 
	: wxPanel(parent, wxID_ANY)
	, m_db(dbRef) {
		
	
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* inspector_grid = new wxFlexGridSizer(3, 2, 5, 5);

	m_st_label_ID = new wxStaticText(this, wxID_ANY, _("ID"));
	m_st_label_ID_num = new wxStaticText(this, wxID_ANY, "");
	m_st_label_name = new wxStaticText(this, wxID_ANY, _("Name"));
	m_tc_name = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	m_st_path = new wxStaticText(this, wxID_ANY, _("Path"));
	m_label_path = new wxStaticText(this, wxID_ANY, _(""));
	
	// 初期状態で隠す
	ViewCtrl(Status::HIDE);

	inspector_grid->Add(m_st_label_ID, 0, wxALL | wxALIGN_CENTER_VERTICAL);
	inspector_grid->Add(m_st_label_ID_num, 0, wxALL | wxALIGN_CENTER_VERTICAL);
	inspector_grid->Add(m_st_label_name, 0, wxALL | wxALIGN_CENTER_VERTICAL);
	inspector_grid->Add(m_tc_name, 0, wxALL | wxALIGN_CENTER_VERTICAL);
	inspector_grid->Add(m_st_path, 0, wxALL | wxALIGN_CENTER_VERTICAL);
	inspector_grid->Add(m_label_path, 0, wxALL | wxALIGN_CENTER_VERTICAL);

	sizer->Add(inspector_grid, 1, wxALL | wxEXPAND, FromDIP(5));
	SetSizer(sizer);
	sizer->Fit(this);

	m_tc_name->Bind(wxEVT_TEXT_ENTER, &Inspector::OnEnterPressed, this);
}

void Inspector::UpdateSelectedCategory(int id, const wxString& name) {
	m_selected_id = id; // ほかに引き渡す用途
	m_selected_name = name;
	m_st_label_ID_num->SetLabel(wxString::Format("%d", id));
	m_tc_name->SetValue(name);
	m_old_name = name; // 現在の値を保持。OnSave で使う

	std::string path = m_db.GetCategoriesPath(id);
	if (path.empty()) {
		m_label_path->SetLabel(_("None"));
	} else {
		m_label_path->SetLabel(wxString::FromUTF8(path));
	}
	if (id == 0) {
		ViewCtrl(Status::HIDE);
	} else {
		ViewCtrl(Status::SHOW);
	}
	// レイアウト崩れ防止
	this->Layout();
}

void Inspector::ViewCtrl(Status status) { // 内容の表示or非表示のコントロール
	bool show_flag = false;
	
	switch (status) {
		case Status::SHOW:
			show_flag = true;
			break;
		case Status::HIDE:
			show_flag = false;
			break;
		default:
			return;
	}

	for (auto ctrl : {
		static_cast<wxWindow*>(m_st_label_ID), 
		static_cast<wxWindow*>(m_st_label_ID_num), 
		static_cast<wxWindow*>(m_st_label_name), 
		static_cast<wxWindow*>(m_tc_name),
		static_cast<wxWindow*>(m_st_path),
		static_cast<wxWindow*>(m_label_path) }) {
		if (ctrl) {
			ctrl->Show(show_flag);
		}
	}
}

void Inspector::OnEnterPressed(wxCommandEvent& event) {
	wxString current_name = m_tc_name->GetValue();
	if (current_name != m_old_name) {
		wxCommandEvent dummy;
		OnSave(dummy);

		m_old_name = current_name; // 保存に成功したので、old_name も更新
	}
}

void Inspector::OnSave(wxCommandEvent& event) { // 保存
	wxString name = m_tc_name->GetValue();

	// 空チェック
	if (name.IsEmpty()){
		wxMessageBox(
		_("Category name is empty"),
		"Error",
		wxOK | wxICON_WARNING,
		this
		    );
		return;
	}
	
	// --- DB 保存処理 ---

	bool result = false;

	result = m_db.UpdateCategories(m_selected_id, std::string(name.utf8_string()));
	if(!result) {
		wxMessageBox(
			_("Unable to save category"),
			"DB Error",
			wxOK | wxICON_ERROR,
			this
		);
		return;
	}

	wxCommandEvent evt(wxEVT_MENU, ID_CATEGORY_UPDATED);
	evt.SetInt(m_selected_id);
	evt.SetString(name);
	wxPostEvent(GetParent(), evt);
}

void Inspector::GetIdAndName(int& id, wxString& name) { // 外部からidとname を取得する
	id = m_selected_id;
	name = m_selected_name;
}
