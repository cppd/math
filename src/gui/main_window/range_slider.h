/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../com/connection.h"

#include <QObject>
#include <QSlider>

#include <vector>

namespace ns::gui::main_window
{
class RangeSlider final : public QObject
{
        Q_OBJECT

private:
        QSlider* slider_min_;
        QSlider* slider_max_;

        double last_min_;
        double last_max_;

        std::vector<Connection> connections_;

        void on_min_value_changed();
        void on_max_value_changed();

        [[nodiscard]] int min_value() const;
        [[nodiscard]] int max_value() const;
        void set_min_value(int);
        void set_max_value(int);

        void range_changed();

public:
        RangeSlider(QSlider* slider_min, QSlider* slider_max);

        void set_range(double min, double max);

Q_SIGNALS:
        void changed(double min, double max);
};
}
