/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "ui_statistics_widget.h"

#include <src/painter/painter.h>

#include <memory>
#include <optional>

namespace ns::gui::painter_window
{
class StatisticsWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::StatisticsWidget ui;

        struct Counters;
        std::unique_ptr<Difference<Counters>> difference_;

public:
        explicit StatisticsWidget(std::chrono::milliseconds update_interval);
        ~StatisticsWidget() override;

        void update(const painter::Statistics& statistics, const std::optional<float>& pixel_max);
};
}
