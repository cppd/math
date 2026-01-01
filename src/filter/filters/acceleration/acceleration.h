/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/filter/filters/noise_model.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::acceleration
{
template <typename T>
[[nodiscard]] std::unique_ptr<Filter<2, T>> create_acceleration_0(
        std::size_t measurement_queue_size,
        T reset_dt,
        T angle_estimation_variance,
        std::optional<T> gate,
        const Init<T>& init,
        T sigma_points_alpha,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const NoiseModel<T>& angle_r_noise_model,
        T fading_memory_alpha);

template <typename T>
[[nodiscard]] std::unique_ptr<Filter<2, T>> create_acceleration_1(
        std::size_t measurement_queue_size,
        T reset_dt,
        T angle_estimation_variance,
        std::optional<T> gate,
        const Init<T>& init,
        T sigma_points_alpha,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const NoiseModel<T>& angle_r_noise_model,
        T fading_memory_alpha);

template <typename T>
[[nodiscard]] std::unique_ptr<Filter<2, T>> create_acceleration_ekf(
        std::size_t measurement_queue_size,
        T reset_dt,
        T angle_estimation_variance,
        std::optional<T> gate,
        const Init<T>& init,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const NoiseModel<T>& angle_r_noise_model,
        T fading_memory_alpha);
}
