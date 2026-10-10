#include <wx/wx.h>
#include "goal.hpp"


Goal::Goal(wxWindow* parent, Database &dbRef)
	: wxFrame(parent, wxID_ANY, wxT("Goal"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , m_db(dbRef)
    , m_timer(this) {
    
    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1550, 600)));

    wxPanel* main_panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* ctrl_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* st_choiceCondition = new wxStaticText(main_panel, wxID_ANY, _("Condition: "));

	m_choiceCondition = new wxChoice(main_panel, wxID_ANY);
	m_choiceCondition->Append(_("All"));
    m_choiceCondition->Append(_("Achieved"));
	m_choiceCondition->Append(_("Unachieved"));
	m_choiceCondition->Append(_("Invalid"));
	m_choiceCondition->SetSelection(2); // 初期表示は Unachieved

    wxStaticText* st_filter_date = new wxStaticText(main_panel, wxID_ANY, _("Show From: "));
    // wxDP_DROPDOWN でカレンダーから日付を選べるようにする。デフォルトは今日
    m_dp_filter_date = new wxDatePickerCtrl(main_panel, wxID_ANY, wxDateTime::Today(),
        wxDefaultPosition, wxDefaultSize, wxDP_DROPDOWN | wxDP_SHOWCENTURY);

    m_btn_add = new wxButton(main_panel, wxID_ANY, _("Add"));
    m_btn_edit = new wxButton(main_panel, wxID_ANY, _("Edit"));

    ctrl_sizer->Add(st_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
	ctrl_sizer->Add(m_choiceCondition, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(st_filter_date, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_dp_filter_date, 0, wxALIGN_CENTER_VERTICAL);
    ctrl_sizer->Add(m_btn_add, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    ctrl_sizer->Add(m_btn_edit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));

    sizer->Add(ctrl_sizer, 0, wxEXPAND | wxALL, FromDIP(10));

    m_ctrl = new wxDataViewCtrl(main_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

    // wxDATAVIEW_COL_SORTABLE は後で付けるかも

    // 列番号は GoalColumn (goal.hpp) を使う。モデルの値の番号と共通
    m_ctrl->AppendTextColumn(_("Name"),
                 Col_Name,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Period"),
                 Col_Period,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Target"),
                 Col_Target,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_RIGHT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Start"),
                 Col_Start,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("End"),
                 Col_End,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_LEFT,
                 wxDATAVIEW_COL_RESIZABLE);

    m_ctrl->AppendTextColumn(_("Active"),
                 Col_Active,
                 wxDATAVIEW_CELL_INERT,
                 wxCOL_WIDTH_AUTOSIZE,
                 wxALIGN_CENTER,
                 wxDATAVIEW_COL_RESIZABLE);

    sizer->Add(m_ctrl, 1, wxEXPAND | wxALL, FromDIP(10));
    main_panel->SetSizer(sizer);
    CenterOnParent();

    Bind(wxEVT_TIMER, &Goal::OnTimer, this);

    m_timer.Start(1000); // 1秒ごとにカウントダウンを更新
}

void Goal::OnTimer(wxTimerEvent& WXUNUSED(event)) {
    long long now_epoch = (long long)wxDateTime::Now().GetTicks();
}
