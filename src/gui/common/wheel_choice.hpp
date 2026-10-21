#pragma once
#include <wx/choice.h>
#include <wx/event.h>

namespace WheelChoice {

// マウスホイールで、wxChoice の選択を 1 つずつ切り替えられるようにする (上に回すと前、下に回すと次)。
// フォーカスがなくても、マウスが重なっていれば効く。切り替えたときは、普通に選んだときと同じ wxEVT_CHOICE を出す
inline void Bind(wxChoice* choice) {
    choice->Bind(wxEVT_MOUSEWHEEL, [choice](wxMouseEvent& event) {
        const int count = static_cast<int>(choice->GetCount());
        if (count <= 1 || event.GetWheelRotation() == 0) {
            event.Skip();
            return;
        }

        // wxMouseEvent の回転は、奥(上)に回すと正。上に回すと、一覧の前の項目へ
        const int current = choice->GetSelection();
        int next = current + (event.GetWheelRotation() > 0 ? -1 : 1);
        if (next < 0) next = 0;
        if (next >= count) next = count - 1;
        if (next == current) return;                 // 端まで来た。そのまま (ウィンドウのスクロールには回さない)

        choice->SetSelection(next);                  // プログラムからの変更では、選択のイベントが出ないので、ここで出す
        wxCommandEvent chosen(wxEVT_CHOICE, choice->GetId());
        chosen.SetEventObject(choice);
        chosen.SetInt(next);
        chosen.SetString(choice->GetString(static_cast<unsigned int>(next)));
        choice->GetEventHandler()->ProcessEvent(chosen);
    });
}

} // namespace WheelChoice
