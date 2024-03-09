/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "init.h"

#include <src/filter/filters/filter.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::speed
{
template <std::size_t N, typename T>
std::unique_ptr<Filter<N, T>> create_speed_1(
        std::size_t measurement_queue_size,
        T reset_dt,
        T angle_estimation_variance,
        std::optional<T> gate,
        const Init<T>& init,
        T sigma_points_alpha,
        T process_variance);

template <std::size_t N, typename T>
std::unique_ptr<Filter<N, T>> create_speed_2(
        std::size_t measurement_queue_size,
        T reset_dt,
        T angle_estimation_variance,
        std::optional<T> gate,
        const Init<T>& init,
        T sigma_points_alpha,
        T process_variance);
}
