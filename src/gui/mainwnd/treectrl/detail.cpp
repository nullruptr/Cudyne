#include <wx/event.h>
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
	// 2列 (ラベル / 値)、行数は項目数に応じて自動
	wxFlexGridSizer* flex = new wxFlexGridSizer(0, 2, FromDIP(8), FromDIP(20));
	flex->AddGrowableCol(1, 1); // 値の列を伸縮させる

	wxStaticText* stat_total_time_all = new wxStaticText(pnlDetail, wxID_ANY, _("Total Time (All-time):"));
	m_result_total_time_all = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_last_executed = new wxStaticText(pnlDetail, wxID_ANY, _("Last Executed:"));
	m_last_executed = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_period_time = new wxStaticText(pnlDetail, wxID_ANY, _("Period Type"));
	m_period_type = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_goal = new wxStaticText(pnlDetail, wxID_ANY, _("Goal"));
	m_goal = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_remaining = new wxStaticText(pnlDetail, wxID_ANY, _("Remaining"));
	m_remaining = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_deadline = new wxStaticText(pnlDetail, wxID_ANY, _("Deadline"));
	m_deadline = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);
	wxStaticText* stat_time_to_deadline = new wxStaticText(pnlDetail, wxID_ANY, _("Time to Deadline"));
	m_time_to_deadline = new wxStaticText(pnlDetail, wxID_ANY, wxEmptyString);

	// ラベルと値をペアで登録
	const wxSizerFlags labelFlags = wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL);
	const wxSizerFlags valueFlags = wxSizerFlags(1).Expand().Align(wxALIGN_CENTER_VERTICAL);
	flex->Add(stat_total_time_all, labelFlags);
	flex->Add(m_result_total_time_all, valueFlags);
	flex->Add(stat_last_executed, labelFlags);
	flex->Add(m_last_executed, valueFlags);
	flex->Add(stat_period_time, labelFlags);
	flex->Add(m_period_type, valueFlags);
	flex->Add(stat_goal, labelFlags);
	flex->Add(m_goal, valueFlags);
	flex->Add(stat_remaining, labelFlags);
	flex->Add(m_remaining, valueFlags);
	flex->Add(stat_deadline, labelFlags);
	flex->Add(m_deadline, valueFlags);
	flex->Add(stat_time_to_deadline, labelFlags);
	flex->Add(m_time_to_deadline, valueFlags);

	rightBoxSizer->Add(flex, 0, wxEXPAND | wxALL, FromDIP(10));

	wxButton* btn_setup_goal = new wxButton(pnlDetail, wxID_ANY, _("Setup Goal"));
	wxButton* btn_setup_plan = new wxButton(pnlDetail, wxID_ANY, _("Setup Plan"));

	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	btnSizer->Add(btn_setup_goal, 0, wxLEFT, FromDIP(8));
	btnSizer->Add(btn_setup_plan, 0);
	rightBoxSizer->Add(btnSizer, 0, wxALL, FromDIP(10));

	// --- about splitter Settigs ---
	splittermain->SplitVertically(pnlTreectrl, pnlDetail); // パネルを左右分割スピリッタに登録
	// ペイン設定 
	splittermain->SetSashPosition(250); // 起動時にpx固定
	splittermain->SetSashGravity(0.0);// Sash を左に固定
	splittermain->SetMinimumPaneSize(250); // 最小サイズ指定

    pnlDetail->SetSizer(rightBoxSizer);
    this->SetSizer(mainSizer);
}

void Detail::OnSetTextOfDetail(wxCommandEvent &event) {
    
}
