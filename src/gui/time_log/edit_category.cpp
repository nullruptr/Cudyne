#include "gui/time_log/edit_category.hpp"
#include <wx/treebase.h>
#include <wx/wx.h>


EditCategory::EditCategory(wxWindow* parent, const wxString& categoryName, Database& db, int parentDbId, int editId = 0)
	: wxDialog(parent, wxID_ANY, "Edit Category Dialog", wxDefaultPosition, wxDefaultSize)
	, m_db(db)
	, m_parentDbId(parentDbId)
	, m_editId(editId)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* CategoryNameSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* CategoryName_st = new wxStaticText(this, wxID_ANY, _("Enter name for new category or directory (dirs is end with \"/\"):"));
	m_categoryNameCtrl = new wxTextCtrl(this, wxID_ANY, categoryName, wxDefaultPosition, wxSize(FromDIP(300), -1)); // 入力欄をあえて広くするために、wxSize で指定

	CategoryNameSizer->Add(CategoryName_st, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(5));
	CategoryNameSizer->Add(m_categoryNameCtrl, 1, wxEXPAND | wxRIGHT, FromDIP(5));

	mainSizer->Add(CategoryNameSizer, 0, wxEXPAND | wxALL, FromDIP(10));


	// 下部
	m_btnSave = new wxButton(this, wxID_ANY, _("Save"));
	m_btnQuit = new wxButton(this, wxID_CANCEL, _("Quit"));

	bottomSizer->AddStretchSpacer();
	bottomSizer->Add(m_btnSave, 0);
	bottomSizer->Add(m_btnQuit, 0);
	mainSizer->Add(bottomSizer, 0, wxEXPAND | wxALL, FromDIP(10));

	SetSizerAndFit(mainSizer);

	m_btnSave->Bind(wxEVT_BUTTON, &EditCategory::OnSave, this);
	m_btnQuit->Bind(wxEVT_BUTTON, &EditCategory::OnQuit, this); // 終了ボタン -> 終了イベント
}

void EditCategory::OnSave(wxCommandEvent& event) { // 保存
    wxString name = m_categoryNameCtrl->GetValue();


    // 文末 "/" 判定 -> （フォルダ判定）
    if (m_editId == 0 && name.EndsWith("/")) {
	m_is_folder = true;
	name.RemoveLast(); // 最後の "/" を消す
    }

    // 空チェック
    if (name.IsEmpty()) {
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

    if (m_editId == 0) {
	    result = m_db.InsertCategories(m_parentDbId, std::string(name.utf8_string()), m_is_folder);
    } else {
	    result = m_db.UpdateCategories(m_editId, std::string(name.utf8_string()));
    }
    if(!result) {
	    wxMessageBox(
			    _("Unable to save category"),
			    "DB Error",
			    wxOK | wxICON_ERROR,
			    this
			);
	    return;
    }

    EndModal(wxID_OK);
    
}

void EditCategory::OnQuit(wxCommandEvent& event){ // 終了
	EndModal(wxID_CANCEL);
}
