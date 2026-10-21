#pragma once
#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/settings.h>
#include <wx/wx.h>

struct AppTheme {
	static wxColor GetBgBlue() { return wxColor(0, 51, 153); }
	static wxColor GetLightBlue() { return wxColor(85, 255, 255); }
	static wxColor GetTextWhite() { return *wxWHITE; }

	// Goal の状態の色 (背景色。文字は既定色のまま読める明るさにする)
	static wxColour GetGoalAchieved()      { return wxColour(198, 239, 206); } // 緑
	static wxColour GetGoalUrgent()        { return wxColour(255, 235, 156); } // 黄
	static wxColour GetGoalBehind()        { return wxColour(255, 217, 179); } // 橙 (急ぎより薄い警告)
	static wxColour GetGoalMissed()       { return wxColour(255, 199, 206); } // 赤
	static wxColour GetGoalNotStarted()    { return wxColour(189, 236, 255); } // 水
	static wxColour GetGoalEndedAchieved() { return wxColour(146, 208, 146); } // 濃い緑 (進行中の達成と区別)
	static wxColour GetGoalEndedMissed()   { return wxColour(240, 140, 140); } // 濃い赤
	static wxColour GetGoalDisabledBg()    { return wxColour(217, 217, 217); } // 灰 (行全体)
	static wxColour GetGoalDisabledText()  { return wxColour(120, 120, 120); }

	static wxFont GetSystemFont() {
		wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
		font.SetPointSize(11);
		return font;
	}
};
