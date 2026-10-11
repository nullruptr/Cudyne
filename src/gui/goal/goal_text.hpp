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
    case GoalEval::State::Comfortable:   return _("On Track");
    }
    return "-";
}

// "Nd hh:mm:ss" (負は先頭に "-")
inline wxString GoalDurationText(long long seconds) {
    const bool neg = seconds < 0;
    long long s = neg ? -seconds : seconds;
    const long long d = s / 86400;
    s %= 86400;
    return wxString::Format("%s%lldd %02lld:%02lld:%02lld", neg ? "-" : "", d, s / 3600, (s % 3600) / 60, s % 60);
}
