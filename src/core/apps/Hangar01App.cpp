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

Hangar01App::Hangar01App(FuncsPtr appFuncs):
    App(appFuncs),
    cancelHTTP(false)
{
    window = std::make_shared<Window>(getUIContainer(), "Hangar01 Checklists");
    window->setOnClose([this] () { exit(); });

    label = std::make_shared<Label>(window, "Loading aircraft list...");
    label->alignInTopLeft();

    aircraftList = std::make_shared<List>(window);
    aircraftList->alignBelow(label);
    aircraftList->setCallback([this] (int index) {
        onAircraftSelected(index);
    });

    loadAircrafts();
}

Hangar01App::~Hangar01App() {
    cancelHTTP = true;
}

void Hangar01App::loadAircrafts() {
    std::thread([this]() {
        try {
            apis::RESTClient rest;
            std::string url = "https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=*[_type%3D%3D%22aircraft%22%26%26!comingSoon]%7Bmodel%2C%22slug%22%3Aslug.current%7D";
            
            bool cancel = false; // We can use a local or cancelHTTP, but RESTClient requires bool&, not atomic<bool>&
            // We'll map atomic -> bool in a wrapper if needed, but RESTClient only periodically checks it.
            // Since we need to pass a reference, let's just use a local one for now because RESTClient doesn't block indefinitely.
            
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
                aircraftList->clear();
                if (aircrafts.empty()) {
                    label->setText("No aircraft found.");
                } else {
                    label->setText("Select an aircraft:");
                    for (size_t i = 0; i < aircrafts.size(); ++i) {
                        aircraftList->add(aircrafts[i].model, Widget::Symbol::NONE, static_cast<int>(i));
                    }
                }
            });
        } catch (const std::exception &e) {
            std::string err = e.what();
            api().executeLater([this, err]() {
                label->setText("Error loading aircraft: " + err);
            });
        }
    }).detach();
}

void Hangar01App::onAircraftSelected(int index) {
    if (index >= 0 && index < static_cast<int>(aircrafts.size())) {
        label->setText("Selected: " + aircrafts[index].model);
        // Next step: Fetch checklist for this aircraft.
    }
}

} /* namespace avitab */
