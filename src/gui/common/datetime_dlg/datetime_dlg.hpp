#pragma once
#include <wx/wx.h>
#include <wx/datectrl.h>

// 日付 (カレンダー)・時刻 (HH:MM)・秒 (SS) を入力するダイアログ。入力は Edit ToDo と同じ。
// 時刻・秒は、Enter かフォーカスが外れたときに、"1300" -> "13:00" のようにそろえる。OK を押したとき、値が不正なら閉じない。
// ボタンは OK (F2) / Cancel (F12)。Esc も Cancel
class DateTimeDlg : public wxDialog {
public:
    DateTimeDlg(wxWindow* parent, const wxString& title, const wxDateTime& initial);

    // OK で閉じたあとに呼ぶ。入力した日時 (ローカル)
    wxDateTime GetDateTime() const;

private:
    wxDatePickerCtrl* m_dp_date;
    wxTextCtrl* m_tc_hhmm;
    wxTextCtrl* m_tc_ss;

    void NormalizeHHMM();   // Enter のとき。不正なら警告して空にする (空のままでは OK が通らない)
    void NormalizeSS();
    void OnOk();            // 不正な入力が残っていたら、警告して閉じない
};
