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

#include "process_filter.h"
#include "simulator.h"

#include "../nees.h"

#include <src/numerical/vector.h>

#include <optional>
#include <string>
#include <vector>

namespace ns::filter::test
{
template <typename T>
class ProcessData final
{
        std::string name_;
        unsigned char color_;
        const ProcessFilter<T>* filter_;

        std::vector<Vector<2, T>> position_;
        std::vector<Vector<2, T>> speed_;

        NeesAverage<2, T> nees_position_;
        NeesAverage<1, T> nees_angle_;
        NeesAverage<1, T> nees_angle_r_;

public:
        ProcessData(std::string name, unsigned char color, const ProcessFilter<T>* filter);

        void save(std::size_t index, const SimulatorPoint<2, T>& point);

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] unsigned char color() const;

        [[nodiscard]] std::string angle_string(const SimulatorPoint<2, T>& point) const;
        [[nodiscard]] std::string nees_string() const;
        [[nodiscard]] const std::vector<Vector<2, T>>& position() const;
        [[nodiscard]] const std::vector<Vector<2, T>>& speed() const;
};
}
