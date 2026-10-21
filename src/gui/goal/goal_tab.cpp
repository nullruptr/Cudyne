#include "goal_tab.hpp"
#include "goal_text.hpp"
#include "core/goal/goal_report.hpp"
#include "core/utils/format_time.hpp"
#include "gui/theme/apptheme.hpp"
#include <ctime>

namespace {
enum Column { COL_GOAL = 0, COL_WINDOW, COL_TARGET, COL_ACHIEVED, COL_STATUS };

wxColour StateColour(GoalEval::State s) {
    switch (s) {
    case GoalEval::State::Achieved:      return AppTheme::GetGoalAchieved();
    case GoalEval::State::Urgent:        return AppTheme::GetGoalUrgent();
    case GoalEval::State::Behind:        return AppTheme::GetGoalBehind();
    case GoalEval::State::Missed:        return AppTheme::GetGoalMissed();
    case GoalEval::State::NotStarted:    return AppTheme::GetGoalNotStarted();
    case GoalEval::State::EndedAchieved: return AppTheme::GetGoalEndedAchieved();
    case GoalEval::State::EndedMissed:   return AppTheme::GetGoalEndedMissed();
    default:                             return wxNullColour; // 余裕あり: 色なし
    }
}
}

GoalTab::GoalTab(wxWindow* parent, Database& dbRef, int category_id)
    : wxPanel(parent, wxID_ANY), m_db(dbRef), m_category_id(category_id) {

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_st_note = new wxStaticText(this, wxID_ANY, "");
    sizer->Add(m_st_note, 0, wxALL, FromDIP(5));

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    m_list->AppendColumn(_("Goal"), wxLIST_FORMAT_LEFT, FromDIP(110));
    m_list->AppendColumn(_("Window"), wxLIST_FORMAT_LEFT, FromDIP(260));
    m_list->AppendColumn(_("Target"), wxLIST_FORMAT_RIGHT, FromDIP(120));
    m_list->AppendColumn(_("Achieved"), wxLIST_FORMAT_RIGHT, FromDIP(80));
    m_list->AppendColumn(_("Status"), wxLIST_FORMAT_LEFT, FromDIP(110));
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}

void GoalTab::Load(bool filtering, const wxDateTime& from_date, const wxDateTime& to_date) {
    const long long now = static_cast<long long>(std::time(nullptr));

    // 期間を絞らないときは、今の瞬間と重なる窓 = 今の窓だけ
    long long from = now, to = now + 1;
    if (filtering) {
        wxDateTime f = from_date;
        f.ResetTime();
        wxDateTime t = to_date;
        t.ResetTime();
        t.Add(wxDateSpan::Day());
        from = static_cast<long long>(f.GetTicks());
        to = static_cast<long long>(t.GetTicks());
    }

    m_list->DeleteAllItems();
    bool any_truncated = false;

    for (const Database::Goal& g : m_db.GetGoalsByTarget(m_category_id, -1)) {
        bool truncated = false;
        const wxString name = g.goal_name.empty() ? wxString::Format("#%d", g.goal_id) : wxString::FromUTF8(g.goal_name);

        for (const GoalReport::Row& row : GoalReport::List(m_db, g, from, to, now, &truncated)) {
            const long item = m_list->InsertItem(m_list->GetItemCount(), name);

            wxString window = TimeUtils::FormatEpochToDate(row.start) + " - ";
            window += row.end == GoalEval::kNone ? wxString("-") : TimeUtils::FormatEpochToDate(row.end);
            m_list->SetItem(item, COL_WINDOW, window);

            wxString target = TimeUtils::FormatSeconds(row.target);
            if (row.overridden) target += " (" + TimeUtils::FormatSeconds(row.normal_target) + ")";
            m_list->SetItem(item, COL_TARGET, target);

            m_list->SetItem(item, COL_ACHIEVED, row.future ? wxString("-") : TimeUtils::FormatSeconds(row.achieved));
            m_list->SetItem(item, COL_STATUS, row.future ? wxString(_("Upcoming")) : GoalStatusText(row.result));

            if (!row.future) {
                const wxColour colour = StateColour(row.result.state);
                if (colour.IsOk()) m_list->SetItemBackgroundColour(item, colour);
            }
        }
        any_truncated = any_truncated || truncated;
    }

    if (any_truncated) {
        m_st_note->SetLabel(wxString::Format(_("Too many windows. Showing the first %d of each goal."), GoalReport::kMaxRowsPerGoal));
    } else if (!filtering) {
        m_st_note->SetLabel(_("Current windows. Turn on \"Filter by period\" to see past windows."));
    } else if (m_list->GetItemCount() == 0) {
        m_st_note->SetLabel(_("No goal windows in this period."));
    } else {
        m_st_note->SetLabel("");
    }
    Layout();
}
