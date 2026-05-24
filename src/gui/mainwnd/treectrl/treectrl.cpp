#include <wx/artprov.h>
#include <wx/event.h>
#include <wx/treebase.h>
#include <wx/wx.h>
#include "gui/mainwnd/mainwnd.hpp"
#include "treectrl.hpp"
#include "gui/time_log/tree_item_data.hpp"

CategoryTree::CategoryTree(wxWindow* parent, Database &dbRef) 
	: wxTreeCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
			wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT) ,
	m_db(dbRef) 
	{
		AddRoot(_("No Database Connected"));
		Bind(wxEVT_TREE_SEL_CHANGED, &CategoryTree::OnItemSelected, this); // アイテム選択時処理
		Bind(wxEVT_CHAR_HOOK, &CategoryTree::OnUpdateKeyDown, this); // F5 入力時

	// コンテキストメニュー（右クリック）イベントをBind
	Bind(wxEVT_CONTEXT_MENU, &CategoryTree::OnContextMenu, this);

	// メニュー項目に対する処理の Bind
	Bind(
		wxEVT_MENU,
		&CategoryTree::OnCreateNewCategory,
		this,
		ID_CREATE	
	);
	//wxID_MENUのメニューにおいて、wxID_EDITが呼ばれたら、CategoryTree::OnEditItemを呼び出すよう指示。

	Bind(
		wxEVT_MENU,
		&CategoryTree::OnEditItem,
		this,
		wxID_EDIT
	);

	Bind(
		wxEVT_MENU,
		&CategoryTree::OnEditParentId,
		this,
		ID_MOVE
	);

	Bind(
		wxEVT_MENU,
		&CategoryTree::OnDeleteItem,
		this,
		wxID_DELETE
	);

    // フォルダアイコンの設定
    m_image_list = new wxImageList(16, 16);
    m_image_list->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16, 16))); // index 0 = ICON_FILE
    m_image_list->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16, 16)));       // index 1 = ICON_FOLDER
    AssignImageList(m_image_list);
}

void CategoryTree::OnItemSelected(wxTreeEvent& event) {
	// クリックされたツリーの項目のwxTreeItemId を取得
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk()) return;
	
	// 紐づけたツリーのID を取得
	TreeItemData* data = static_cast<TreeItemData*>(GetItemData(item));
	if (data) {
		wxCommandEvent evt(wxEVT_MENU, ID_CATEGORY_SELECTED);
		evt.SetInt(data->GetId());            // カテゴリIDを格納
		evt.SetString(GetItemText(item));     // カテゴリ名を格納
	        // イベントの発生源を自分に設定
		evt.SetEventObject(this);
		// 親ウィンドウへ向かってイベントを投げる
		wxPostEvent(GetParent(), evt);
	}
}

void CategoryTree::OnUpdateKeyDown(wxKeyEvent& event) { // F5 でアップデート
	if (event.GetKeyCode() == WXK_F5) {
		UpdateTreeData();
		return;
	}
	event.Skip();
}

void CategoryTree::UpdateTreeData() {
	std::vector<Database::Category> categories;

	if (!m_db.GetAllCategories(categories)){ // 全件格納。無理ならreturn。
		return;
	}

	DeleteAllItems(); // ツリーリセット

	wxTreeItemId root = AddRoot(wxString::FromUTF8(m_db.GetDBPath())); // 起点rootを作成
	BuildTree(0, root, categories);
	Expand(root);
}

void CategoryTree::BuildTree(
		int parentId,
		wxTreeItemId parentNode,
		const std::vector<Database::Category>& categories
		) {
	for (const auto& c : categories){ // カテゴリ数だけループ
		if (c.parent_id == parentId){ // DB のparent_id と 引数の数値が同値の時
			wxString displayName = wxString::FromUTF8(c.name.c_str()); // UTF8 で読み込み
			
			// アイコンをセット
			int icon = c.is_folder ? ICON_FOLDER : ICON_FILE;

			wxTreeItemId node = AppendItem(parentNode, displayName, icon, icon, new TreeItemData(c.id)); // 親の場所に子の名前でツリー登録。同時にID(c.id) をデータとして紐付け。
			BuildTree(c.id, node, categories); // 再起処理。自分のIDとノードを渡して、カテゴリ全件も渡す。子が存在したらぶら下げる。
		}
	}
}

void CategoryTree::OnCreateNewCategory(wxCommandEvent &event){
	// 現在選択されているアイテムを取得
	wxTreeItemId parent = GetSelection();

	// 無効な場合は処理しない
	if (!parent.IsOk()){
		return;
	}

	int parent_db_id = 0;
	if (parent.IsOk()){
		TreeItemData* parentData = (TreeItemData*)GetItemData(parent);
		if (parentData) { // データがあるとき(nullptr出ないとき)
			parent_db_id = parentData->GetId();
		}
		// parentData が nullptr (rootノード)なら、parent_db_id = 0 のまま
	}
	// 新規ノード名
	wxString newCategoryName = wxT("New Category");


	// 編集用画面呼び出し
	EditCategory dlg(this,
			newCategoryName,
			m_db,
			parent_db_id,
			0 // 新規のため、編集ID なし
			); // 編集用ダイアログ呼び出し
	if (dlg.ShowModal() == wxID_OK) {
		// 保存成功 -> ツリー再読み込み
		UpdateTreeData();
	}
	
}

void CategoryTree::OnEditParentId(wxCommandEvent& event) {
	wxTreeItemId item = GetSelection(); // 選択されたアイテム情報取得

	if (!item.IsOk()) return; 
	
	TreeItemData* data = (TreeItemData*)GetItemData(item); 
	if (!data) return;

	int id = data->GetId();
	wxString name = GetItemText(item);


	EditParentId dlg(this,
			id,
			m_db
			);

	if (dlg.ShowModal() == wxID_OK) {
		UpdateTreeData(); // ツリー再読み込み
	}
}

void CategoryTree::OnEditItem(wxCommandEvent& event){
	// ツリーでクリックされた内容を取得
	wxTreeItemId item = GetSelection();

	if (!item.IsOk()){
		return;
	}

	TreeItemData* data = (TreeItemData*)GetItemData(item);

	if(!data){
		return;
	}

	int id = data->GetId();
	wxString currentName = GetItemText(item);

	EditCategory dlg(this,
			currentName,
			m_db,
			0, // parentId は編集時不要
			id // editId
			);

	if (dlg.ShowModal() == wxID_OK) {
		UpdateTreeData(); // ツリー再読み込み
	}
}

void CategoryTree::OnDeleteItem(wxCommandEvent& event){ // 削除および非表示処理
	wxTreeItemId item = GetSelection(); // 選択されたアイテム情報取得

	if (!item.IsOk()) return; 
	
	TreeItemData* data = (TreeItemData*)GetItemData(item); 
	if (!data) return;

	int id = data->GetId();
	wxString name = GetItemText(item);

	if (m_db.HasRecords(id)) {
		int ans = wxMessageBox(
				wxString::Format(_("'%s' has records.\n Are you sure you want to hide this category?"), name),
				_("Check"),
				wxYES_NO | wxICON_QUESTION,
				this
				);
		if (ans == wxYES) {
			m_db.HideCategory(id);
			UpdateTreeData();
		}
	} else {
		// 記録がない -> 物理削除(現状は非表示フラグをたてている)
		int ans = wxMessageBox(
				wxString::Format(_("Are you sure you want to delete '%s'?"), name),
				_("Check"),
				wxYES_NO | wxICON_WARNING,
				this
				);
		if (ans == wxYES) {
			m_db.HideCategory(id);
			UpdateTreeData();
		}
	}

}


void CategoryTree::OnContextMenu(wxContextMenuEvent& event) {

	// 1. クリックされた位置（スクリーン座標）を取得
	wxPoint point = event.GetPosition();

	// 2. スクリーン座標をクライアント座標（ウィンドウ内の相対座標）に変換
	if (point != wxDefaultPosition) {
		wxPoint clientPoint = ScreenToClient(point);
		
		// 3. その座標にアイテムがあるか判定
		int flags;
		wxTreeItemId item = HitTest(clientPoint, flags);

		// 4. 有効なアイテムの上であれば、それを選択状態にする
		if (item.IsOk()) {
		SelectItem(item);
		}
	}

	// 右クリックメニューの作成
	wxMenu menu;
	menu.Append(ID_CREATE, _("Create New Category"));
	menu.Append(wxID_EDIT, _("Edit"));
	menu.Append(ID_MOVE, _("Move"));
	menu.Append(wxID_DELETE, _("Delete"));
	
	// マウスカーソル位置にメニューを表示する
	PopupMenu(&menu);
}
