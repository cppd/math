/*
Copyright (C) 2017-2022 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "ui_lighting_widget.h"

#include <src/color/color.h>
#include <src/view/view.h>

#include <tuple>

namespace ns::gui::main_window
{
class LightingWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::LightingWidget ui_;

        view::View* view_ = nullptr;

        const double daylight_min_cct_;
        const double daylight_max_cct_;
        const double blackbody_min_t_;
        const double blackbody_max_t_;

        double intensity_;
        color::Spectrum spectrum_;
        color::Color rgb_;

        void send_color();

        void on_intensity_changed();
        void on_daylight_changed();
        void on_blackbody_changed();
        void on_d65_toggled();
        void on_daylight_toggled();
        void on_blackbody_a_toggled();
        void on_blackbody_toggled();

public:
        LightingWidget();

        void set_view(view::View* view);

        std::tuple<color::Spectrum, color::Color> color() const;
        color::Spectrum spectrum() const;
        color::Color rgb() const;
};
}
