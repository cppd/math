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

#include "../filter/estimation.h"
#include "../filter/measurement.h"
#include "../filter/time_point.h"

#include <src/color/rgb8.h>

#include <string>
#include <vector>

namespace ns::filter::test::process
{
template <typename T>
class Process
{
public:
        virtual ~Process() = default;

        virtual void update(const Measurements<2, T>& m, const Estimation<T>& estimation) = 0;

        [[nodiscard]] virtual const std::string& name() const = 0;
        [[nodiscard]] virtual color::RGB8 color() const = 0;

        [[nodiscard]] virtual std::string consistency_string() const = 0;
        [[nodiscard]] virtual const std::vector<TimePoint<2, T>>& positions() const = 0;
        [[nodiscard]] virtual const std::vector<TimePoint<2, T>>& positions_p() const = 0;
        [[nodiscard]] virtual const std::vector<TimePoint<1, T>>& speeds() const = 0;
        [[nodiscard]] virtual const std::vector<TimePoint<1, T>>& speeds_p() const = 0;
};
}
