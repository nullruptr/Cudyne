#pragma once
#include <wx/wx.h>
#include "core/db/database.hpp"
#include "gui/todo/todo_panel.hpp"

// ToDo ウィンドウ。中身は ToDoPanel
class ToDo : public wxFrame{

public:
	ToDo(wxWindow* parent, Database& db);
};
