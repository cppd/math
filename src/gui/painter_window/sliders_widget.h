/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <QLabel>
#include <QObject>
#include <QSlider>
#include <QWidget>

#include <unordered_map>
#include <vector>

namespace ns::gui::painter_window
{
class SlidersWidget final : public QWidget
{
        Q_OBJECT

private:
        struct Slider final
        {
                QLabel* label;
                unsigned number;
        };

        std::unordered_map<QSlider*, Slider> sliders_;
        std::vector<int> slider_positions_;

        void on_slider_changed(int);

public:
        explicit SlidersWidget(const std::vector<int>& screen_size);

        void set(const std::vector<int>& positions);

Q_SIGNALS:
        void changed(const std::vector<int>& positions);
};
}
