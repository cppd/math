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

#include "filter.h"

#include <src/com/error.h>
#include <src/filter/core/test/measurements.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::core::test::filters
{
template <typename Filter>
bool filter_update(
        Filter* const filter,
        const Measurements<typename Filter::Type>& m,
        const std::optional<typename Filter::Type> gate)
{
        ASSERT(filter);
        ASSERT(m.position || m.speed);

        if (m.position)
        {
                if (m.speed)
                {
                        filter->update_position_speed(
                                m.position->value, m.position->variance, m.speed->value, m.speed->variance, gate);
                        return true;
                }

                filter->update_position(m.position->value, m.position->variance, gate);
                return true;
        }

        if (m.speed)
        {
                filter->update_speed(m.speed->value, m.speed->variance, gate);
                return true;
        }

        return false;
}

template <typename T>
struct PredictInfo final
{
        std::optional<numerical::Matrix<2, 2, T>> f;
        std::optional<numerical::Vector<2, T>> x;
        std::optional<numerical::Matrix<2, 2, T>> p;
};

template <typename Filter>
[[nodiscard]] UpdateInfo<typename Filter::Type> make_update_info(
        const PredictInfo<typename Filter::Type>& predict,
        const Filter& filter)
{
        return {
                .position = filter.position(),
                .position_stddev = std::sqrt(filter.position_p()),
                .speed = filter.speed(),
                .speed_stddev = std::sqrt(filter.speed_p()),
                .predict_f = predict.f,
                .predict_x = predict.x,
                .predict_p = predict.p,
                .update_x = filter.position_speed(),
                .update_p = filter.position_speed_p(),
        };
}
}
