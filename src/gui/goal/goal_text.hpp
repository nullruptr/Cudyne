#pragma once
#include <cstdlib>
#include <wx/intl.h>
#include <wx/string.h>
#include "core/db/database.hpp"
#include "core/goal/goal_eval.hpp"

// goal の表示用文字列
inline wxString GoalPeriodText(const Database::Goal& g) {
    switch (g.period_type) {
    case 0: return _("Daily");
    case 1: return _("Weekly");
    case 2: return _("Monthly");
    case 3: return wxString::Format(_("Every %d Days"), g.period_n);
    case 4: return _("Undefined");
    case 5: return wxString::Format(_("Rolling %d Days"), g.period_n);
    default: return "-";
    }
}

inline wxString GoalStateText(GoalEval::State s) {
    switch (s) {
    case GoalEval::State::Disabled:      return _("Disabled");
    case GoalEval::State::EndedAchieved: return _("Ended (Achieved)");
    case GoalEval::State::EndedMissed:   return _("Ended (Missed)");
    case GoalEval::State::Achieved:      return _("Achieved");
    case GoalEval::State::NotStarted:    return _("Not Started");
    case GoalEval::State::Missed:        return _("Missed");
    case GoalEval::State::Urgent:        return _("Urgent");
    case GoalEval::State::Behind:        return _("Behind");
    case GoalEval::State::Comfortable:   return _("On Track");
    }
    return "-";
}

// 状態セルの文字列。Behind は達成率 - 経過率 (pt) を添える (例: "Behind -12pt")
inline wxString GoalStatusText(const GoalEval::Result& r) {
    if (r.state == GoalEval::State::Behind && r.has_pace) {
        return wxString::Format("%s %.0fpt", GoalStateText(r.state), r.achieved_pct - r.elapsed_pct);
    }
    return GoalStateText(r.state);
}

// 経過率と達成率 (例: "Elapsed 80% / Achieved 55%")。計算できないときは "-"
inline wxString GoalPaceText(const GoalEval::Result& r) {
    if (!r.has_pace) return "-";
    return wxString::Format(_("Elapsed %.0f%% / Achieved %.0f%%"), r.elapsed_pct, r.achieved_pct);
}

// "Nd hh:mm:ss" (負は先頭に "-")
inline wxString GoalDurationText(long long seconds) {
    const bool neg = seconds < 0;
    long long s = neg ? -seconds : seconds;
    const long long d = s / 86400;
    s %= 86400;
    return wxString::Format("%s%lldd %02lld:%02lld:%02lld", neg ? "-" : "", d, s / 3600, (s % 3600) / 60, s % 60);
}
