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

#include "noise_model.h"

#include <src/filter/core/consistency.h>
#include <src/filter/core/test/measurements.h>

#include <memory>
#include <optional>

namespace ns::filter::core::test::filters
{
template <typename T>
struct UpdateInfo final
{
        T x;
        T x_stddev;
        T v;
        T v_stddev;
};

template <typename T>
class Filter
{
public:
        virtual ~Filter() = default;

        virtual void reset() = 0;

        [[nodiscard]] virtual std::optional<UpdateInfo<T>> update(const Measurements<T>& m) = 0;

        [[nodiscard]] virtual const NormalizedSquared<T>& nees() const = 0;
};

template <typename T>
std::unique_ptr<Filter<T>> create_ekf(
        T init_v,
        T init_v_variance,
        const NoiseModel<T>& noise_model,
        T fading_memory_alpha,
        T reset_dt,
        std::optional<T> gate);

template <typename T>
std::unique_ptr<Filter<T>> create_h_infinity(
        T init_v,
        T init_v_variance,
        const NoiseModel<T>& noise_model,
        T fading_memory_alpha,
        T reset_dt,
        std::optional<T> gate);

template <typename T>
std::unique_ptr<Filter<T>> create_info(T init_v, T init_v_variance, const NoiseModel<T>& noise_model, T reset_dt);

template <typename T>
std::unique_ptr<Filter<T>> create_ukf(
        T init_v,
        T init_v_variance,
        const NoiseModel<T>& noise_model,
        T fading_memory_alpha,
        T reset_dt,
        std::optional<T> gate);
}
