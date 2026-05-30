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
#include "gui_toolkit/widgets/Page.h"

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

    enum class ViewState {
        AIRCRAFT_LIST,
        CHECKLIST
    };

    enum class TierFilter {
        QUICK,
        NORMAL,
        FULL
    };

    // --- Checklist item widget row ---
    struct ItemRow {
        std::shared_ptr<Checkbox> checkbox;
        std::shared_ptr<Label> infoLabel;  // only if item has info; hidden by default
        int itemIndex;   // index into flat items list
        bool hasInfo;
    };

    // --- UI widgets ---
    std::shared_ptr<Window> window;

    // Aircraft list view
    std::shared_ptr<List> aircraftList;

    // Checklist view
    std::shared_ptr<Label> titleLabel;
    std::shared_ptr<Button> btnQuick;
    std::shared_ptr<Button> btnNormal;
    std::shared_ptr<Button> btnFull;
    std::shared_ptr<Label> infoBar;  // bottom tooltip bar for (i) content
    lv_style_t infoBarStyle;         // opaque background style for info bar

    // All dynamic labels/checkboxes in the checklist scroll area
    std::vector<std::shared_ptr<Label>> sectionLabels;
    std::vector<ItemRow> itemRows;

    // --- State ---
    ViewState currentView;
    TierFilter currentTier;
    std::vector<Aircraft> aircrafts;
    std::vector<Phase> phases;
    int selectedAircraftIndex;
    std::atomic<bool> cancelHTTP;

    // --- Methods ---
    void showAircraftList();
    void showChecklist();
    void rebuildChecklistContent();

    void loadAircraftList();
    void loadChecklist(const std::string &slug);

    void onAircraftSelected(int index);
    void onTierChanged(TierFilter tier);
    void onBackButton();
    void onItemChecked(int flatIndex, bool checked);
    void onInfoTapped(int flatIndex);

    void clearChecklistWidgets();
    void setDebug(const std::string &msg);
    bool itemPassesTierFilter(const std::string &tier) const;
};

} /* namespace avitab */
