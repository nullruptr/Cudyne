#pragma once
#include "core/db/database.hpp"
#include <wx/button.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/wx.h>

class EditCategory : public wxDialog {
public:
	EditCategory(wxWindow* parent, const wxString& categoryName, Database& db, int parentDbId, int editId);
	void OnQuit(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
private:
	wxTextCtrl* m_categoryNameCtrl;
	wxButton* m_btnSave;
	wxButton* m_btnQuit;

	Database& m_db;
	int m_parentDbId;
	int m_editId; // 0で新規、それ以外なら編集モード
	bool m_is_folder = false; // 作るものがフォルダか否か
};
