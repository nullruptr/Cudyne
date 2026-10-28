#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/cmdline.h>
#include <wx/wx.h>
#include "main.hpp"
#include "gui/mainwnd/mainwnd.hpp"
#include "version.hpp"

// enum は，mainwnd.hpp に移植


wxIMPLEMENT_APP(MBApp);
wxString MBApp::s_path;

// エントリ
bool MBApp::OnInit() {
    if (argc > 1) {
        s_path = argv[1];
    }
	locale.Init(wxLANGUAGE_ENGLISH); // 使用言語設定
	locale.AddCatalogLookupPathPrefix("locale");
	locale.AddCatalog("mb");
	Mainwnd *mainwnd = new Mainwnd(nullptr); 
	mainwnd->SetTitle(AppVersion::Get()); // Build habits the sticky way
	mainwnd->Show(true);
	return true;
}

wxString& MBApp::Getter(wxString path) {
    return s_path;
}
