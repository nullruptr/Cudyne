#include <wx/sizer.h>
#include <wx/versioninfo.h>
#include <wx/wx.h>
#include <wx/splitter.h>
#include "detail.hpp"

Detail::Detail(wxWindow* parent, Database &dbRef, int id)
	: wxFrame(parent, wxID_ANY, wxT("Detail"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
      , m_db(dbRef)
      , m_id(id) {
    
    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1550, 600)));

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL); // メインサイザー
	wxSplitterWindow* splittermain = new wxSplitterWindow(this, wxID_ANY); // 分割作成
	mainSizer->Add(splittermain, 1, wxEXPAND, 0); // 分割をメインサイザーへ追加

	// パネル設定
	wxPanel* pnlTreectrl = new wxPanel(splittermain, wxID_ANY);
	wxPanel* pnlDetail = new wxPanel(splittermain, wxID_ANY);

	m_categoryTree = new CategoryTree(pnlTreectrl, m_db);

    // left
	wxBoxSizer* treeSizer = new wxBoxSizer(wxVERTICAL);
	treeSizer->Add(m_categoryTree, 1, wxEXPAND);
	pnlTreectrl->SetSizer(treeSizer);

    //right
    wxBoxSizer* rightBoxSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* detail_st = new wxStaticText(pnlDetail, wxID_ANY, _("Detail"));
    rightBoxSizer->Add(detail_st, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 10);
	wxFlexGridSizer* flex = new wxFlexGridSizer(4, 2, FromDIP(8), FromDIP(20));

	// --- about splitter Settigs ---
	splittermain->SplitVertically(pnlTreectrl, pnlDetail); // パネルを左右分割スピリッタに登録

    pnlDetail->SetSizer(rightBoxSizer);
    this->SetSizer(mainSizer);
}
