#include "plan_check_panel.hpp"
#include "core/utils/format_time.hpp"
#include "gui/theme/apptheme.hpp"

namespace {
enum Column { COL_GOAL = 0, COL_WINDOW, COL_REMAINING, COL_PLANNED, COL_GAP, COL_VERDICT };

wxString ShortTime(long long epoch) {
    return wxDateTime(static_cast<time_t>(epoch)).Format("%m/%d %H:%M");
}

// 符号つきの時間 ("+01:30:00" / "-00:45:00")
wxString SignedDuration(long long seconds) {
    return (seconds < 0 ? "-" : "+") + TimeUtils::FormatSeconds(seconds < 0 ? -seconds : seconds);
}

wxColour VerdictColour(PlanCheck::Verdict v) {
    switch (v) {
    case PlanCheck::Verdict::Achieved:   return AppTheme::GetGoalEndedAchieved(); // 濃い緑: すでに達成
    case PlanCheck::Verdict::Covered:    return AppTheme::GetGoalAchieved();      // 薄い緑: 予定でまかなえる
    case PlanCheck::Verdict::Short:      return AppTheme::GetGoalUrgent();        // 黄: 予定が足りない
    case PlanCheck::Verdict::Impossible: return AppTheme::GetGoalMissed();        // 赤: 間に合わない
    case PlanCheck::Verdict::Overdue:    return AppTheme::GetGoalEndedMissed();   // 濃い赤: 期限切れ
    }
    return wxNullColour;
}

wxString VerdictText(const PlanCheck::Result& r) {
    switch (r.verdict) {
    case PlanCheck::Verdict::Achieved:   return _("Achieved");
    case PlanCheck::Verdict::Covered:    return _("Covered by plans");
    case PlanCheck::Verdict::Short:      return wxString::Format(_("Short by %s"), TimeUtils::FormatSeconds(r.shortfall));
    case PlanCheck::Verdict::Impossible: return wxString::Format(_("Impossible (%s left)"), TimeUtils::FormatSeconds(r.time_left));
    case PlanCheck::Verdict::Overdue:    return _("Overdue");
    }
    return "-";
}
}

PlanCheckPanel::PlanCheckPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    const int gap = FromDIP(6);

    wxStaticText* title = new wxStaticText(this, wxID_ANY, _("Can the goals be achieved with these plans?"));
    wxFont font = title->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(font);

    m_st_note = new wxStaticText(this, wxID_ANY, "");
    m_st_summary = new wxStaticText(this, wxID_ANY, "");

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->AppendColumn(_("Goal"), wxLIST_FORMAT_LEFT, FromDIP(120));
    m_list->AppendColumn(_("Window"), wxLIST_FORMAT_LEFT, FromDIP(170));
    m_list->AppendColumn(_("Remaining"), wxLIST_FORMAT_RIGHT, FromDIP(80));
    m_list->AppendColumn(_("Planned"), wxLIST_FORMAT_RIGHT, FromDIP(80));
    m_list->AppendColumn(_("Plan - Remaining"), wxLIST_FORMAT_RIGHT, FromDIP(100));
    m_list->AppendColumn(_("Verdict"), wxLIST_FORMAT_LEFT, FromDIP(160));

    sizer->Add(title, 0, wxALL, gap);
    sizer->Add(m_st_note, 0, wxLEFT | wxRIGHT, gap);
    sizer->Add(m_st_summary, 0, wxALL, gap);
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}

void PlanCheckPanel::SetBusy() {
    m_st_summary->SetLabel(_("Checking..."));
    Layout();
}

void PlanCheckPanel::SetItems(const std::vector<PlanFeasibility::Item>& items, const wxString& note) {
    m_list->DeleteAllItems();
    m_st_note->SetLabel(note);

    int achieved = 0, covered = 0, shortage = 0, impossible = 0, overdue = 0, unknown = 0;

    for (const PlanFeasibility::Item& item : items) {
        wxString name = wxString::FromUTF8(item.goal_name);
        if (!item.subject.empty()) name += " (" + wxString::FromUTF8(item.subject) + ")";
        const long row = m_list->InsertItem(m_list->GetItemCount(), name);

        if (!item.applicable) {
            // 判定できない Goal (ToDo に設定したもの)
            m_list->SetItem(row, COL_VERDICT, wxString::FromUTF8(item.reason));
            m_list->SetItemTextColour(row, wxColour(120, 120, 120));
            ++unknown;
            continue;
        }

        const PlanCheck::Result& r = item.result;
        const GoalReport::Row& w = item.window;
        m_list->SetItem(row, COL_WINDOW, ShortTime(w.start) + " - " + (w.end == GoalEval::kNone ? wxString("-") : ShortTime(w.end)));
        m_list->SetItem(row, COL_REMAINING, TimeUtils::FormatSeconds(r.remaining));
        m_list->SetItem(row, COL_PLANNED, TimeUtils::FormatSeconds(r.planned));
        m_list->SetItem(row, COL_GAP, r.remaining > 0 ? SignedDuration(r.planned - r.remaining) : wxString("-"));
        m_list->SetItem(row, COL_VERDICT, VerdictText(r));

        const wxColour colour = VerdictColour(r.verdict);
        if (colour.IsOk()) m_list->SetItemBackgroundColour(row, colour);

        switch (r.verdict) {
        case PlanCheck::Verdict::Achieved:   ++achieved;   break;
        case PlanCheck::Verdict::Covered:    ++covered;    break;
        case PlanCheck::Verdict::Short:      ++shortage;   break;
        case PlanCheck::Verdict::Impossible: ++impossible; break;
        case PlanCheck::Verdict::Overdue:    ++overdue;    break;
        }
    }

    if (items.empty()) {
        m_st_summary->SetLabel(_("No goal windows to check"));
    } else {
        wxString summary = wxString::Format(_("Covered %d / Short %d / Impossible %d / Overdue %d / Achieved %d"),
                                            covered, shortage, impossible, overdue, achieved);
        if (unknown > 0) summary += wxString::Format(_(" / Not checked %d"), unknown);
        m_st_summary->SetLabel(summary);
    }
    Layout();
}
