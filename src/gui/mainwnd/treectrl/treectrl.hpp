#pragma once
#include <wx/event.h>
#include <wx/image.h>
#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include "core/db/database.hpp"
#include "gui/time_log/edit_category.hpp"
#include "gui/time_log/edit_parent_id.hpp"

class CategoryTree : public wxTreeCtrl {
public:
	CategoryTree(wxWindow* parent, Database& db);
	void OnItemSelected(wxTreeEvent& event);
	void OnUpdateKeyDown(wxKeyEvent& event); // F5 で更新処理を受信
	void UpdateTreeData();
	void BuildTree(
			int parentId,
			wxTreeItemId parentNode,
			const std::vector<Database::Category> &categories
		      );
	void OnCreateNewCategory(wxCommandEvent& event);
	void OnCreateNewRecord(wxCommandEvent& event);
	void OnEditParentId(wxCommandEvent& event); // parent id の編集
	void OnEditItem(wxCommandEvent& event);
	void OnDeleteItem(wxCommandEvent& event); // イベント削除
	void OnContextMenu(wxContextMenuEvent& event);
private:
	Database& m_db;
	wxTreeItemId m_context_item;
	wxImageList* m_image_list;
	enum {
	    ICON_FILE = 0,
	    ICON_FOLDER = 1
	};
};

enum{
	ID_TREE_EDIT,
	ID_CREATE_NEW_CATEGORY,
	ID_CREATE_NEW_RECORD,
	ID_MOVE
};
