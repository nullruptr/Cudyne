#include "todo.hpp"

ToDo::ToDo(wxWindow* parent, Database &dbRef)
	: wxFrame(parent, wxID_ANY, wxT("ToDo"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT) {

    // ウィンドウが初期化された後に FromDIP しないとクラッシュする
    SetSize(FromDIP(wxSize(1550, 600)));

    // 「Start Recording」のイベントは、このウィンドウの親 (メインウィンドウ) に送る
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new ToDoPanel(this, dbRef, -1, false, parent), 1, wxEXPAND);
    SetSizer(sizer);
    CenterOnParent();
}
