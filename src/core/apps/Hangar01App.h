/*
 *   AviTab - Aviator's Virtual Tablet
 *   Copyright (C) 2018-2026 Folke Will and Avitab Contributors
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include "App.h"
#include "gui_toolkit/widgets/Window.h"
#include "gui_toolkit/widgets/Label.h"
#include "gui_toolkit/widgets/List.h"
#include "gui_toolkit/widgets/Button.h"
#include "gui_toolkit/widgets/Checkbox.h"
#include "gui_toolkit/widgets/DropDownList.h"

namespace avitab {

class Hangar01App: public App {
public:
    Hangar01App(FuncsPtr appFuncs);
    ~Hangar01App() override;

private:
    // --- Data model ---
    struct ChecklistItem {
        std::string key;
        std::string label;
        std::string action;
        std::string info;
        std::string tier; // "quick", "normal", "full"
    };

    struct Group {
        std::string key;
        std::string title;
        std::vector<ChecklistItem> items;
    };

    struct Phase {
        std::string key;
        std::string title;
        std::vector<Group> groups;
    };

    struct Aircraft {
        std::string model;
        std::string slug;
    };

    // --- View state ---
    enum class ViewState {
        AIRCRAFT_LIST,
        PHASE_LIST,
        GROUP_LIST,
        CHECKLIST_ITEMS
    };

    // --- Tier filter ---
    enum class TierFilter {
        QUICK,   // show only "quick" items
        NORMAL,  // show "quick" + "normal" items
        FULL     // show all items
    };

    // --- UI widgets ---
    std::shared_ptr<Window> window;
    std::shared_ptr<Label> titleLabel;
    std::shared_ptr<Label> subtitleLabel;
    std::shared_ptr<List> mainList;
    std::shared_ptr<DropDownList> tierDropdown;

    // Checklist item view widgets
    std::vector<std::shared_ptr<Checkbox>> checkboxWidgets;
    std::vector<std::shared_ptr<Label>> actionLabels;
    std::vector<std::shared_ptr<Label>> infoLabels;

    // --- State ---
    ViewState currentView;
    TierFilter currentTier;
    std::vector<Aircraft> aircrafts;
    std::vector<Phase> phases;
    int selectedAircraftIndex;
    int selectedPhaseIndex;
    int selectedGroupIndex;
    std::atomic<bool> cancelHTTP;

    // --- Methods ---
    void showAircraftList();
    void showPhaseList();
    void showGroupList();
    void showChecklistItems();

    void loadAircraftList();
    void loadChecklist(const std::string &slug);

    void onAircraftSelected(int index);
    void onPhaseSelected(int index);
    void onGroupSelected(int index);
    void onTierChanged();
    void onBackButton();

    void clearChecklistWidgets();
    void rebuildUI();
    bool itemPassesTierFilter(const std::string &tier) const;
};

} /* namespace avitab */
