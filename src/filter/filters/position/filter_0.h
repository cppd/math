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

#include <src/filter/core/update_info.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
class Filter0
{
public:
        virtual ~Filter0() = default;

        virtual void reset(const numerical::Vector<N, T>& position, const numerical::Vector<N, T>& variance) = 0;

        virtual numerical::Matrix<N, N, T> predict(T dt, const NoiseModel<T>& noise_model, T fading_memory_alpha) = 0;

        [[nodiscard]] virtual core::UpdateInfo<N, T> update(
                const numerical::Vector<N, T>& position,
                const numerical::Vector<N, T>& variance,
                std::optional<T> gate) = 0;

        [[nodiscard]] virtual numerical::Vector<N, T> position() const = 0;
        [[nodiscard]] virtual numerical::Matrix<N, N, T> position_p() const = 0;

        [[nodiscard]] virtual const numerical::Vector<N, T>& x() const = 0;
        [[nodiscard]] virtual const numerical::Matrix<N, N, T>& p() const = 0;

        [[nodiscard]] virtual numerical::Vector<N, T> x_to_position(const numerical::Vector<N, T>& x) const = 0;
        [[nodiscard]] virtual numerical::Vector<N, T> p_to_position_p(const numerical::Matrix<N, N, T>& p) const = 0;
};

template <std::size_t N, typename T>
std::unique_ptr<Filter0<N, T>> create_filter_0(T theta);
}
