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

#include <src/filter/core/test/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::core::test::filters::info
{
template <typename T>
class FilterInfo
{
public:
        using Type = T;

        virtual ~FilterInfo() = default;

        virtual void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& i) = 0;

        [[nodiscard]] virtual numerical::Matrix<2, 2, T> predict(
                T dt,
                const NoiseModel<T>& noise_model,
                T fading_memory_alpha) = 0;

        virtual void update_position(T position, T position_variance, std::optional<T> gate) = 0;

        virtual void update_position_speed(
                T position,
                T position_variance,
                T speed,
                T speed_variance,
                std::optional<T> gate) = 0;

        virtual void update_speed(T speed, T speed_variance, std::optional<T> gate) = 0;

        [[nodiscard]] virtual T position() const = 0;
        [[nodiscard]] virtual T position_p() const = 0;

        [[nodiscard]] virtual numerical::Vector<2, T> position_speed() const = 0;
        [[nodiscard]] virtual numerical::Matrix<2, 2, T> position_speed_p() const = 0;

        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;
};

template <typename T>
[[nodiscard]] std::unique_ptr<FilterInfo<T>> create_filter_info();
}
