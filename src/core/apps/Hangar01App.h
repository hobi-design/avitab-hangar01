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

namespace avitab {

class Hangar01App: public App {
public:
    Hangar01App(FuncsPtr appFuncs);
    ~Hangar01App() override;

private:
    std::shared_ptr<Window> window;
    std::shared_ptr<Label> label;
    std::shared_ptr<List> aircraftList;

    struct Aircraft {
        std::string model;
        std::string slug;
    };
    std::vector<Aircraft> aircrafts;
    std::atomic<bool> cancelHTTP;

    void loadAircrafts();
    void onAircraftSelected(int index);
};

} /* namespace avitab */
