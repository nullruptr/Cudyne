#include "datetime_dlg.hpp"
#include "core/utils/format_time.hpp"
#include <wx/dateevt.h>

DateTimeDlg::DateTimeDlg(wxWindow* parent, const wxString& title, const wxDateTime& initial)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE) {

    const int gap = FromDIP(8);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    // 日付 / 時刻 / 秒 / = Now を 1 行に並べる (Edit ToDo と同じ並び)
    m_dp_date = new wxDatePickerCtrl(this, wxID_ANY, initial, wxDefaultPosition, wxDefaultSize,
                                     wxDP_DROPDOWN | wxDP_SHOWCENTURY);
    m_tc_hhmm = new wxTextCtrl(this, wxID_ANY, initial.Format("%H:%M"), wxDefaultPosition,
                               FromDIP(wxSize(55, -1)), wxTE_PROCESS_ENTER);
    m_tc_ss = new wxTextCtrl(this, wxID_ANY, initial.Format("%S"), wxDefaultPosition,
                             FromDIP(wxSize(40, -1)), wxTE_PROCESS_ENTER);
    wxButton* btn_now = new wxButton(this, wxID_ANY, _("= Now"));

    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(m_dp_date, 0, wxALIGN_CENTER_VERTICAL);
    row->Add(m_tc_hhmm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row->Add(m_tc_ss, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    row->Add(btn_now, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
    sizer->Add(new wxStaticText(this, wxID_ANY, _("Date    Time (HH:MM)    Seconds")), 0, wxLEFT | wxRIGHT | wxTOP, gap);
    sizer->Add(row, 0, wxEXPAND | wxALL, gap);

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    wxButton* btn_ok = new wxButton(this, wxID_OK, _("OK (F2)"));
    buttons->AddButton(btn_ok);
    buttons->AddButton(new wxButton(this, wxID_CANCEL, _("Cancel (F12)")));
    buttons->Realize();
    btn_ok->SetDefault();
    sizer->Add(buttons, 0, wxEXPAND | wxALL, gap);

    SetSizerAndFit(sizer);
    CenterOnParent();

    btn_now->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        const wxDateTime now = wxDateTime::Now();
        m_dp_date->SetValue(now);
        m_tc_hhmm->SetValue(now.Format("%H:%M"));
        m_tc_ss->SetValue(now.Format("%S"));
    });
    m_tc_hhmm->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { NormalizeHHMM(); });
    m_tc_ss->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { NormalizeSS(); });
    btn_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnOk(); });

    // F2 = OK、F12 = Cancel。入力欄にフォーカスがあっても効くよう、子に渡る前のキーで拾う (Esc は標準で Cancel)
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
        switch (event.GetKeyCode()) {
        case WXK_F2:  OnOk(); break;
        case WXK_F12: EndModal(wxID_CANCEL); break;
        default:      event.Skip(); break;
        }
    });

    m_tc_hhmm->SetFocus();
    m_tc_hhmm->SelectAll();
}

// 不正な入力は、警告して空にする (空のままでは OK が通らない)
void DateTimeDlg::NormalizeHHMM() {
    const wxString normalized = TimeUtils::ParseHHMM(m_tc_hhmm->GetValue());
    if (normalized.IsEmpty()) wxMessageBox(_("Invalid time"), "Error", wxOK | wxICON_WARNING, this);
    m_tc_hhmm->SetValue(normalized);
}

void DateTimeDlg::NormalizeSS() {
    const wxString normalized = TimeUtils::ParseSS(m_tc_ss->GetValue());
    if (normalized.IsEmpty()) wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING, this);
    m_tc_ss->SetValue(normalized);
}

void DateTimeDlg::OnOk() {
    // OK を押す直前の入力も、検証してからそろえる
    const wxString hhmm = TimeUtils::ParseHHMM(m_tc_hhmm->GetValue());
    const wxString ss = TimeUtils::ParseSS(m_tc_ss->GetValue());
    if (hhmm.IsEmpty()) {
        wxMessageBox(_("Invalid time"), "Error", wxOK | wxICON_WARNING, this);
        m_tc_hhmm->SetFocus();
        return; // 閉じない
    }
    if (ss.IsEmpty()) {
        wxMessageBox(_("Invalid seconds"), "Error", wxOK | wxICON_WARNING, this);
        m_tc_ss->SetFocus();
        return;
    }
    m_tc_hhmm->SetValue(hhmm);
    m_tc_ss->SetValue(ss);
    EndModal(wxID_OK);
}

wxDateTime DateTimeDlg::GetDateTime() const {
    long hh = 0, mm = 0, sec = 0;
    m_tc_hhmm->GetValue().BeforeFirst(':').ToLong(&hh);
    m_tc_hhmm->GetValue().AfterFirst(':').ToLong(&mm);
    m_tc_ss->GetValue().ToLong(&sec);

    wxDateTime dt = m_dp_date->GetValue();
    dt.SetHour(static_cast<wxDateTime::wxDateTime_t>(hh));
    dt.SetMinute(static_cast<wxDateTime::wxDateTime_t>(mm));
    dt.SetSecond(static_cast<wxDateTime::wxDateTime_t>(sec));
    return dt;
}
