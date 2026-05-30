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
#include <src/lv_core/lv_style.h>

namespace avitab {

// API URLs
static const std::string SANITY_BASE =
    "https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=";

static const std::string AIRCRAFT_LIST_QUERY =
    "*%5B_type%3D%3D%22aircraft%22%26%26!comingSoon%5D%7Bmodel%2C%22slug%22%3Aslug.current%7D";

static const std::string CHECKLIST_QUERY_PREFIX =
    "*%5B_type%3D%3D%22aircraft%22%26%26slug.current%3D%3D%24slug%5D%5B0%5D%7Bmodel%2Cslug%2Cmanufacturer%2Cchecklist%7D&%24slug=%22";

static const std::string CHECKLIST_QUERY_SUFFIX = "%22";

// Colors for LVGL recolor syntax: #RRGGBB text#
static const std::string COL_PHASE   = "#00BFFF ";   // bright cyan for phase headers
static const std::string COL_GROUP   = "#AAAAAA ";   // grey for group sub-headers
static const std::string COL_ACTION  = "#00E676 ";   // green accent for actions
static const std::string COL_DIM     = "#555555 ";   // dimmed for checked items
static const std::string COL_END     = "#";          // close color tag

// ─────────────────────────────────────────────────────────────────────────────

Hangar01App::Hangar01App(FuncsPtr appFuncs):
    App(appFuncs),
    currentView(ViewState::AIRCRAFT_LIST),
    currentTier(TierFilter::NORMAL),
    selectedAircraftIndex(-1),
    cancelHTTP(false)
{
    logger::info("[H01] Hangar01App constructor");

    window = std::make_shared<Window>(getUIContainer(), "HANGAR01");
    window->setOnClose([this] () { exit(); });
    window->addSymbol(Widget::Symbol::LEFT, [this] () { onBackButton(); });

    // Start with aircraft list
    showAircraftList();
}

Hangar01App::~Hangar01App() {
    cancelHTTP = true;
    logger::info("[H01] destructor");
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void Hangar01App::setDebug(const std::string &msg) {
    logger::info("[H01] %s", msg.c_str());
}

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

// formatItemText is no longer used — text is built inline in rebuildChecklistContent

// ─── Network: aircraft list ──────────────────────────────────────────────────

void Hangar01App::loadAircraftList() {
    setDebug("Fetching aircraft list...");

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
                setDebug("Loaded " + std::to_string(aircrafts.size()) + " aircraft");
                showAircraftList();
            });
        } catch (const std::exception &e) {
            std::string err = e.what();
            api().executeLater([this, err]() {
                setDebug("FETCH FAIL: " + err);
            });
        }
    }).detach();
}

// ─── Network: checklist ──────────────────────────────────────────────────────

void Hangar01App::loadChecklist(const std::string &slug) {
    setDebug("Fetching checklist for " + slug);

    std::thread([this, slug]() {
        try {
            apis::RESTClient rest;
            std::string url = SANITY_BASE + CHECKLIST_QUERY_PREFIX + slug + CHECKLIST_QUERY_SUFFIX;
            bool cancel = false;
            std::string res = rest.get(url, cancel);
            if (cancelHTTP) return;

            auto json = nlohmann::json::parse(res);
            auto result = json.value("result", nlohmann::json::object());

            if (result.is_null()) {
                api().executeLater([this, slug]() {
                    setDebug("NULL result for " + slug);
                });
                return;
            }

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

            api().executeLater([this, parsedPhases, slug]() {
                phases = parsedPhases;
                setDebug("Loaded " + std::to_string(phases.size()) + " phases for " + slug);
                showChecklist();
            });
        } catch (const std::exception &e) {
            std::string err = e.what();
            api().executeLater([this, err]() {
                setDebug("FAIL: " + err);
            });
        }
    }).detach();
}

// ─── View: Aircraft list ─────────────────────────────────────────────────────

void Hangar01App::showAircraftList() {
    currentView = ViewState::AIRCRAFT_LIST;
    clearChecklistWidgets();

    // Hide checklist-specific widgets
    if (titleLabel) titleLabel->setVisible(false);
    if (btnQuick) btnQuick->setVisible(false);
    if (btnNormal) btnNormal->setVisible(false);
    if (btnFull) btnFull->setVisible(false);
    if (infoBar) infoBar->setVisible(false);

    // Create or show the aircraft list
    if (!aircraftList) {
        aircraftList = std::make_shared<List>(window);
        // Fill the full window width and height
        int w = window->getContentWidth();
        int h = window->getContentHeight();
        aircraftList->setDimensions(w, h);
        aircraftList->alignInTopLeft();
    }

    aircraftList->setVisible(true);
    aircraftList->clear();

    if (aircrafts.empty()) {
        // Still loading — trigger the fetch
        loadAircraftList();
    } else {
        for (size_t i = 0; i < aircrafts.size(); ++i) {
            aircraftList->add(aircrafts[i].model, Widget::Symbol::RIGHT, static_cast<int>(i));
        }
    }

    aircraftList->setCallback([this] (int index) { onAircraftSelected(index); });
}

// ─── View: Checklist (single scrollable page) ────────────────────────────────

void Hangar01App::showChecklist() {
    currentView = ViewState::CHECKLIST;

    // Hide aircraft list
    if (aircraftList) aircraftList->setVisible(false);

    int contentW = window->getContentWidth();

    // Title label showing aircraft name
    if (!titleLabel) {
        titleLabel = std::make_shared<Label>(window, "");
        titleLabel->setAllowColors(true);
        titleLabel->setLongMode(true);
        titleLabel->setDimensions(contentW, 20);
        titleLabel->alignInTopLeft();
    }
    titleLabel->setVisible(true);
    if (selectedAircraftIndex >= 0 && selectedAircraftIndex < static_cast<int>(aircrafts.size())) {
        titleLabel->setText(aircrafts[selectedAircraftIndex].model);
    }

    // Tier toggle buttons
    if (!btnQuick) {
        btnQuick = std::make_shared<Button>(window, "Quick");
        btnQuick->setToggleable(true);
        btnQuick->setCallback([this](const Button &) { onTierChanged(TierFilter::QUICK); });
        btnQuick->alignBelow(titleLabel, 4);
    }
    if (!btnNormal) {
        btnNormal = std::make_shared<Button>(window, "Normal");
        btnNormal->setToggleable(true);
        btnNormal->setCallback([this](const Button &) { onTierChanged(TierFilter::NORMAL); });
        btnNormal->alignRightOf(btnQuick, 4);
    }
    if (!btnFull) {
        btnFull = std::make_shared<Button>(window, "Full");
        btnFull->setToggleable(true);
        btnFull->setCallback([this](const Button &) { onTierChanged(TierFilter::FULL); });
        btnFull->alignRightOf(btnNormal, 4);
    }
    btnQuick->setVisible(true);
    btnNormal->setVisible(true);
    btnFull->setVisible(true);

    // Update toggle states
    btnQuick->setToggleState(currentTier == TierFilter::QUICK);
    btnNormal->setToggleState(currentTier == TierFilter::NORMAL);
    btnFull->setToggleState(currentTier == TierFilter::FULL);

    // Info bar at the bottom — shows (i) content when tapped
    if (!infoBar) {
        infoBar = std::make_shared<Label>(window, "");
        infoBar->setAllowColors(true);
        infoBar->setLongMode(true);
        infoBar->setDimensions(contentW, 30);
        infoBar->alignInBottomLeft();

        // Style: solid dark background so text doesn't mix with checklist items
        lv_style_copy(&infoBarStyle, &lv_style_plain);
        infoBarStyle.body.main_color = lv_color_hex(0x1A1A1A);
        infoBarStyle.body.grad_color = lv_color_hex(0x1A1A1A);
        infoBarStyle.body.opa = LV_OPA_COVER;
        infoBarStyle.body.border.width = 1;
        infoBarStyle.body.border.color = lv_color_hex(0x333333);
        infoBarStyle.body.border.part = LV_BORDER_TOP;
        infoBarStyle.body.padding.left = 6;
        infoBarStyle.body.padding.right = 6;
        infoBarStyle.body.padding.top = 6;
        infoBarStyle.body.padding.bottom = 4;
        infoBarStyle.text.color = lv_color_hex(0xCCCCCC);
        lv_obj_set_style(infoBar->obj(), &infoBarStyle);
    }
    infoBar->setVisible(false);
    infoBar->setText("");

    // Build the checklist content
    rebuildChecklistContent();
}

void Hangar01App::rebuildChecklistContent() {
    // Clear previous dynamic widgets
    clearChecklistWidgets();

    int contentW = window->getContentWidth();

    // We'll stack everything below the tier buttons
    std::shared_ptr<Widget> lastWidget = btnQuick;
    int flatIndex = 0;

    for (const auto &phase : phases) {
        // Phase header — colored, uppercase-style
        auto phaseLabel = std::make_shared<Label>(window, "");
        phaseLabel->setAllowColors(true);
        phaseLabel->setLongMode(true);
        phaseLabel->setDimensions(contentW, 22);
        phaseLabel->setText(COL_PHASE + "--- " + phase.title + " ---" + COL_END);
        phaseLabel->alignBelow(lastWidget, 10);
        sectionLabels.push_back(phaseLabel);
        lastWidget = phaseLabel;

        for (const auto &group : phase.groups) {
            // Group sub-header — smaller, grey
            auto groupLabel = std::make_shared<Label>(window, "");
            groupLabel->setAllowColors(true);
            groupLabel->setLongMode(true);
            groupLabel->setDimensions(contentW, 18);
            groupLabel->setText(COL_GROUP + group.title + COL_END);
            groupLabel->alignBelow(lastWidget, 6);
            sectionLabels.push_back(groupLabel);
            lastWidget = groupLabel;

            for (const auto &item : group.items) {
                if (!itemPassesTierFilter(item.tier)) {
                    flatIndex++;
                    continue;
                }

                // Build checkbox text: "Label - ACTION" with action in green
                std::string cbText = item.label;
                if (!item.action.empty()) {
                    cbText += " - " + item.action;
                }
                // Add (i) indicator if item has info
                if (!item.info.empty()) {
                    cbText += " (i)";
                }

                auto cb = std::make_shared<Checkbox>(window, cbText);
                cb->alignBelow(lastWidget, 3);
                cb->setDimensions(contentW, 22);

                int idx = flatIndex;
                bool hasInfo = !item.info.empty();
                std::string infoText = item.info;

                cb->setCallback([this, idx, hasInfo, infoText](bool checked) {
                    onItemChecked(idx, checked);
                    // Show info bar with item description when tapped
                    if (hasInfo && infoBar) {
                        infoBar->setText(infoText);
                        infoBar->setVisible(true);
                    }
                });

                ItemRow row;
                row.checkbox = cb;
                row.infoLabel = nullptr;
                row.itemIndex = flatIndex;
                row.hasInfo = hasInfo;
                itemRows.push_back(row);

                lastWidget = cb;
                flatIndex++;
            }
        }
    }

    setDebug("Rendered checklist: " + std::to_string(itemRows.size()) + " visible items");
}

// ─── Callbacks ───────────────────────────────────────────────────────────────

void Hangar01App::onAircraftSelected(int index) {
    if (index < 0 || index >= static_cast<int>(aircrafts.size())) return;
    selectedAircraftIndex = index;
    setDebug("Selected " + aircrafts[index].model);
    loadChecklist(aircrafts[index].slug);
}

void Hangar01App::onTierChanged(TierFilter tier) {
    currentTier = tier;
    setDebug("Tier changed to " + std::to_string(static_cast<int>(tier)));

    // Update toggle visual states
    if (btnQuick) btnQuick->setToggleState(currentTier == TierFilter::QUICK);
    if (btnNormal) btnNormal->setToggleState(currentTier == TierFilter::NORMAL);
    if (btnFull) btnFull->setToggleState(currentTier == TierFilter::FULL);

    // Rebuild content with new filter
    rebuildChecklistContent();
}

void Hangar01App::onBackButton() {
    if (currentView == ViewState::CHECKLIST) {
        showAircraftList();
    } else {
        exit();
    }
}

void Hangar01App::onItemChecked(int flatIndex, bool checked) {
    // Find the row and dim it if checked
    for (auto &row : itemRows) {
        if (row.itemIndex == flatIndex && row.checkbox) {
            if (checked) {
                // Dim the checkbox by setting opacity via LVGL style
                lv_obj_set_opa_scale_enable(row.checkbox->obj(), true);
                lv_obj_set_opa_scale(row.checkbox->obj(), LV_OPA_50);
            } else {
                lv_obj_set_opa_scale_enable(row.checkbox->obj(), true);
                lv_obj_set_opa_scale(row.checkbox->obj(), LV_OPA_COVER);
            }
            break;
        }
    }
}

void Hangar01App::onInfoTapped(int /*flatIndex*/) {
    // Info text is set directly by the click handler via captured string
    // This method exists for future extensibility
}

// ─── Cleanup ─────────────────────────────────────────────────────────────────

void Hangar01App::clearChecklistWidgets() {
    sectionLabels.clear();
    itemRows.clear();
}

} /* namespace avitab */
