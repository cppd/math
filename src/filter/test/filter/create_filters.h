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

#include "test_filter.h"

#include "acceleration/acceleration.h"
#include "position/position.h"
#include "position/position_estimation.h"
#include "position/position_variance.h"

#include <memory>
#include <vector>

namespace ns::filter::test::filter
{
template <typename T>
struct Test final
{
        std::unique_ptr<position::PositionVariance<2, T>> position_variance;

        std::vector<std::unique_ptr<position::Position<2, T>>> positions_0;
        std::vector<std::unique_ptr<position::Position<2, T>>> positions_1;
        std::vector<std::unique_ptr<position::Position<2, T>>> positions_2;

        std::vector<std::unique_ptr<acceleration::Acceleration<T>>> accelerations;

        std::vector<std::unique_ptr<TestFilter<T>>> directions_1_0;
        std::vector<std::unique_ptr<TestFilter<T>>> directions_1_1;
        std::vector<std::unique_ptr<TestFilter<T>>> directions_2_1;

        std::vector<std::unique_ptr<TestFilter<T>>> speeds_1;
        std::vector<std::unique_ptr<TestFilter<T>>> speeds_2;

        std::unique_ptr<position::PositionEstimation<T>> position_estimation;
};

template <typename T>
Test<T> create_data();
}
