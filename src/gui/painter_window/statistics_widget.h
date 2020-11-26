/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "difference.h"
#include "pixels.h"

#include "ui_statistics_widget.h"

#include <memory>

namespace gui::painter_window
{
class StatisticsWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::StatisticsWidget ui;

        const Pixels* m_pixels;

        const double m_pixel_count;

        struct Counters;
        std::unique_ptr<Difference<Counters>> m_difference;

public:
        StatisticsWidget(const Pixels* pixels, std::chrono::milliseconds update_interval);
        ~StatisticsWidget() override;

        void update();
};
}
