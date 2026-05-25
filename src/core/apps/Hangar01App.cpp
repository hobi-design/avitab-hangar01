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
#include "Hangar01App.h"
#include "Logger.h"
#include "charts/RESTClient.h"
#include <nlohmann/json.hpp>
#include <thread>

namespace avitab {

// ─── API URLs ────────────────────────────────────────────────────────────────
static const std::string SANITY_BASE =
    "https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=";

// URL-encoded: *[_type=="aircraft"&&!comingSoon]{model,"slug":slug.current}
static const std::string AIRCRAFT_LIST_QUERY =
    "*%5B_type%3D%3D%22aircraft%22%26%26!comingSoon%5D%7Bmodel%2C%22slug%22%3Aslug.current%7D";

// URL-encoded prefix: *[_type=="aircraft"&&slug.current==$slug][0]{model,slug,manufacturer,checklist}&$slug="
static const std::string CHECKLIST_QUERY_PREFIX =
    "*%5B_type%3D%3D%22aircraft%22%26%26slug.current%3D%3D%24slug%5D%5B0%5D%7Bmodel%2Cslug%2Cmanufacturer%2Cchecklist%7D&%24slug=%22";

// Closing quote for slug parameter
static const std::string CHECKLIST_QUERY_SUFFIX = "%22";

// ─── Constructor / Destructor ────────────────────────────────────────────────

Hangar01App::Hangar01App(FuncsPtr appFuncs):
    App(appFuncs),
    currentView(ViewState::AIRCRAFT_LIST),
    currentTier(TierFilter::NORMAL),
    selectedAircraftIndex(-1),
    selectedPhaseIndex(-1),
    selectedGroupIndex(-1),
    cancelHTTP(false)
{
    window = std::make_shared<Window>(getUIContainer(), "HANGAR01");
    window->setOnClose([this] () { exit(); });

    // Back button in window header
    window->addSymbol(Widget::Symbol::LEFT, [this] () { onBackButton(); });

    titleLabel = std::make_shared<Label>(window, "Loading aircraft...");
    titleLabel->alignInTopLeft();

    subtitleLabel = std::make_shared<Label>(window, "");
    subtitleLabel->alignBelow(titleLabel);
    subtitleLabel->setVisible(false);

    mainList = std::make_shared<List>(window);
    mainList->alignBelow(titleLabel, 5);

    loadAircraftList();
}

Hangar01App::~Hangar01App() {
    cancelHTTP = true;
}

// ─── Tier filter logic ───────────────────────────────────────────────────────

bool Hangar01App::itemPassesTierFilter(const std::string &tier) const {
    switch (currentTier) {
        case TierFilter::QUICK:
            return (tier == "quick");
        case TierFilter::NORMAL:
            return (tier == "quick" || tier == "normal");
        case TierFilter::FULL:
            return true;
    }
    return true;
}

// ─── Network: Load aircraft list ─────────────────────────────────────────────

void Hangar01App::loadAircraftList() {
    std::thread([this]() {
        try {
            apis::RESTClient rest;
            std::string url = SANITY_BASE + AIRCRAFT_LIST_QUERY;
            bool cancel = false;
            std::string res = rest.get(url, cancel);
            if (cancelHTTP) return;

            auto json = nlohmann::json::parse(res);
            auto results = json.value("result", nlohmann::json::array());

            std::vector<Aircraft> parsed;
            for (const auto& item : results) {
                Aircraft ac;
                ac.model = item.value("model", std::string("Unknown"));
                ac.slug = item.value("slug", std::string(""));
                parsed.push_back(ac);
            }

            api().executeLater([this, parsed]() {
                aircrafts = parsed;
                showAircraftList();
            });
        } catch (const std::exception &e) {
            std::string err = e.what();
            api().executeLater([this, err]() {
                titleLabel->setText("Error: " + err);
            });
        }
    }).detach();
}

// ─── Network: Load single aircraft checklist ─────────────────────────────────

void Hangar01App::loadChecklist(const std::string &slug) {
    titleLabel->setText("Loading checklist...");
    mainList->clear();
    mainList->setVisible(false);
    subtitleLabel->setVisible(false);
    clearChecklistWidgets();

    std::thread([this, slug]() {
        try {
            apis::RESTClient rest;
            std::string url = SANITY_BASE + CHECKLIST_QUERY_PREFIX + slug + CHECKLIST_QUERY_SUFFIX;
            bool cancel = false;
            std::string res = rest.get(url, cancel);
            if (cancelHTTP) return;

            auto json = nlohmann::json::parse(res);
            auto result = json.value("result", nlohmann::json::object());
            auto checklistArr = result.value("checklist", nlohmann::json::array());

            std::vector<Phase> parsedPhases;
            for (const auto& phaseJson : checklistArr) {
                Phase phase;
                phase.key = phaseJson.value("_key", std::string(""));
                phase.title = phaseJson.value("title", std::string("Untitled Phase"));

                auto groupsArr = phaseJson.value("groups", nlohmann::json::array());
                for (const auto& groupJson : groupsArr) {
                    Group group;
                    group.key = groupJson.value("_key", std::string(""));
                    group.title = groupJson.value("title", std::string("Untitled Group"));

                    auto itemsArr = groupJson.value("items", nlohmann::json::array());
                    for (const auto& itemJson : itemsArr) {
                        ChecklistItem item;
                        item.key = itemJson.value("_key", std::string(""));
                        item.label = itemJson.value("label", std::string(""));
                        item.action = itemJson.value("action", std::string(""));
                        item.info = itemJson.value("info", std::string(""));
                        item.tier = itemJson.value("tier", std::string("normal"));
                        group.items.push_back(item);
                    }
                    phase.groups.push_back(group);
                }
                parsedPhases.push_back(phase);
            }

            api().executeLater([this, parsedPhases]() {
                phases = parsedPhases;
                showPhaseList();
            });
        } catch (const std::exception &e) {
            std::string err = e.what();
            api().executeLater([this, err]() {
                titleLabel->setText("Error: " + err);
            });
        }
    }).detach();
}

// ─── View: Aircraft list ─────────────────────────────────────────────────────

void Hangar01App::showAircraftList() {
    currentView = ViewState::AIRCRAFT_LIST;
    clearChecklistWidgets();

    if (tierDropdown) {
        tierDropdown->setVisible(false);
    }
    subtitleLabel->setVisible(false);

    mainList->setVisible(true);
    mainList->clear();
    mainList->alignBelow(titleLabel, 5);

    if (aircrafts.empty()) {
        titleLabel->setText("No aircraft found.");
    } else {
        titleLabel->setText("Select Aircraft:");
        for (size_t i = 0; i < aircrafts.size(); ++i) {
            mainList->add(aircrafts[i].model, Widget::Symbol::RIGHT, static_cast<int>(i));
        }
    }

    mainList->setCallback([this] (int index) { onAircraftSelected(index); });
}

// ─── View: Phase list ────────────────────────────────────────────────────────

void Hangar01App::showPhaseList() {
    currentView = ViewState::PHASE_LIST;
    clearChecklistWidgets();

    if (tierDropdown) {
        tierDropdown->setVisible(false);
    }
    subtitleLabel->setVisible(false);

    mainList->setVisible(true);
    mainList->clear();
    mainList->alignBelow(titleLabel, 5);

    if (selectedAircraftIndex >= 0 && selectedAircraftIndex < static_cast<int>(aircrafts.size())) {
        titleLabel->setText(aircrafts[selectedAircraftIndex].model + " - Phases:");
    } else {
        titleLabel->setText("Phases:");
    }

    if (phases.empty()) {
        titleLabel->setText("No checklist data available.");
    } else {
        for (size_t i = 0; i < phases.size(); ++i) {
            mainList->add(phases[i].title, Widget::Symbol::RIGHT, static_cast<int>(i));
        }
    }

    mainList->setCallback([this] (int index) { onPhaseSelected(index); });
}

// ─── View: Group list ────────────────────────────────────────────────────────

void Hangar01App::showGroupList() {
    currentView = ViewState::GROUP_LIST;
    clearChecklistWidgets();

    if (tierDropdown) {
        tierDropdown->setVisible(false);
    }
    subtitleLabel->setVisible(false);

    mainList->setVisible(true);
    mainList->clear();
    mainList->alignBelow(titleLabel, 5);

    if (selectedPhaseIndex >= 0 && selectedPhaseIndex < static_cast<int>(phases.size())) {
        titleLabel->setText(phases[selectedPhaseIndex].title + " - Groups:");
        const auto &groups = phases[selectedPhaseIndex].groups;
        for (size_t i = 0; i < groups.size(); ++i) {
            mainList->add(groups[i].title, Widget::Symbol::RIGHT, static_cast<int>(i));
        }
    } else {
        titleLabel->setText("No groups available.");
    }

    mainList->setCallback([this] (int index) { onGroupSelected(index); });
}

// ─── View: Checklist items ───────────────────────────────────────────────────

void Hangar01App::showChecklistItems() {
    currentView = ViewState::CHECKLIST_ITEMS;
    clearChecklistWidgets();
    mainList->setVisible(false);

    if (selectedPhaseIndex < 0 || selectedPhaseIndex >= static_cast<int>(phases.size())) return;
    const auto &groups = phases[selectedPhaseIndex].groups;
    if (selectedGroupIndex < 0 || selectedGroupIndex >= static_cast<int>(groups.size())) return;

    const auto &group = groups[selectedGroupIndex];
    titleLabel->setText(group.title);

    // Show tier filter dropdown
    if (!tierDropdown) {
        std::vector<std::string> tierChoices = {"Quick", "Normal", "Full"};
        tierDropdown = std::make_shared<DropDownList>(window, tierChoices);
        tierDropdown->setSelectAction([this] () { onTierChanged(); });
    }
    tierDropdown->setVisible(true);
    tierDropdown->alignBelow(titleLabel, 5);

    // Subtitle showing current filter
    subtitleLabel->setVisible(true);
    switch (currentTier) {
        case TierFilter::QUICK:  subtitleLabel->setText("Showing: Quick items only"); break;
        case TierFilter::NORMAL: subtitleLabel->setText("Showing: Quick + Normal items"); break;
        case TierFilter::FULL:   subtitleLabel->setText("Showing: All items"); break;
    }
    subtitleLabel->alignBelow(tierDropdown, 5);

    // Render filtered checklist items
    std::shared_ptr<Widget> lastWidget = subtitleLabel;

    for (const auto &item : group.items) {
        if (!itemPassesTierFilter(item.tier)) continue;

        // Checkbox with "LABEL .............. ACTION" format
        std::string checkText = item.label;
        if (!item.action.empty()) {
            checkText += " — " + item.action;
        }

        auto cb = std::make_shared<Checkbox>(window, checkText);
        cb->alignBelow(lastWidget, 3);
        cb->setCallback([](bool) { /* visual toggle only */ });
        checkboxWidgets.push_back(cb);
        lastWidget = cb;

        // Info label (if present)
        if (!item.info.empty()) {
            auto infoLbl = std::make_shared<Label>(window, "  " + item.info);
            infoLbl->setAllowColors(true);
            infoLbl->setLongMode(true);
            infoLbl->alignBelow(lastWidget, 1);
            infoLabels.push_back(infoLbl);
            lastWidget = infoLbl;
        }
    }

    if (checkboxWidgets.empty()) {
        auto emptyLbl = std::make_shared<Label>(window, "No items for this filter level.");
        emptyLbl->alignBelow(lastWidget, 10);
        infoLabels.push_back(emptyLbl);
    }
}

// ─── Callbacks ───────────────────────────────────────────────────────────────

void Hangar01App::onAircraftSelected(int index) {
    if (index < 0 || index >= static_cast<int>(aircrafts.size())) return;
    selectedAircraftIndex = index;
    loadChecklist(aircrafts[index].slug);
}

void Hangar01App::onPhaseSelected(int index) {
    if (index < 0 || index >= static_cast<int>(phases.size())) return;
    selectedPhaseIndex = index;
    showGroupList();
}

void Hangar01App::onGroupSelected(int index) {
    if (selectedPhaseIndex < 0 || selectedPhaseIndex >= static_cast<int>(phases.size())) return;
    const auto &groups = phases[selectedPhaseIndex].groups;
    if (index < 0 || index >= static_cast<int>(groups.size())) return;
    selectedGroupIndex = index;
    showChecklistItems();
}

void Hangar01App::onTierChanged() {
    if (!tierDropdown) return;
    int sel = tierDropdown->getSelectedIndex();
    switch (sel) {
        case 0: currentTier = TierFilter::QUICK; break;
        case 1: currentTier = TierFilter::NORMAL; break;
        case 2: currentTier = TierFilter::FULL; break;
        default: currentTier = TierFilter::NORMAL; break;
    }
    // Re-render checklist items with new filter
    showChecklistItems();
}

void Hangar01App::onBackButton() {
    switch (currentView) {
        case ViewState::AIRCRAFT_LIST:
            // Already at top level, exit the app
            exit();
            break;
        case ViewState::PHASE_LIST:
            // Go back to aircraft list
            showAircraftList();
            break;
        case ViewState::GROUP_LIST:
            // Go back to phase list
            showPhaseList();
            break;
        case ViewState::CHECKLIST_ITEMS:
            // Go back to group list
            showGroupList();
            break;
    }
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void Hangar01App::clearChecklistWidgets() {
    checkboxWidgets.clear();
    actionLabels.clear();
    infoLabels.clear();
}

void Hangar01App::rebuildUI() {
    switch (currentView) {
        case ViewState::AIRCRAFT_LIST: showAircraftList(); break;
        case ViewState::PHASE_LIST: showPhaseList(); break;
        case ViewState::GROUP_LIST: showGroupList(); break;
        case ViewState::CHECKLIST_ITEMS: showChecklistItems(); break;
    }
}

} /* namespace avitab */
