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

#include "estimation.h"
#include "measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>
#include <string>

namespace ns::filter::filters
{
template <std::size_t N, typename T>
struct UpdateInfo final
{
        numerical::Vector<N, T> position;
        numerical::Vector<N, T> position_p;
        T speed;
        T speed_p;
};

template <std::size_t N, typename T>
struct UpdateDetails final
{
        T time;

        std::optional<numerical::Matrix<N, N, T>> predict_f;
        std::optional<numerical::Vector<N, T>> predict_x;
        std::optional<numerical::Matrix<N, N, T>> predict_p;

        numerical::Vector<N, T> update_x;
        numerical::Matrix<N, N, T> update_p;
};

template <std::size_t N, typename T, std::size_t ORDER>
struct UpdateInfoPosition final
{
        UpdateInfo<N, T> info;
        UpdateDetails<N * (1 + ORDER), T> details;
};

template <std::size_t N, typename T, std::size_t ORDER>
class FilterPosition
{
private:
        static constexpr std::size_t D = N * (1 + ORDER);

public:
        virtual ~FilterPosition() = default;

        [[nodiscard]] virtual std::optional<UpdateInfoPosition<N, T, ORDER>> update(const Measurements<N, T>& m) = 0;

        [[nodiscard]] virtual std::string consistency_string() const = 0;

        [[nodiscard]] virtual bool empty() const = 0;

        [[nodiscard]] virtual numerical::Vector<N, T> position() const = 0;
        [[nodiscard]] virtual numerical::Matrix<N, N, T> position_p() const = 0;
        [[nodiscard]] virtual numerical::Vector<N, T> velocity() const = 0;
        [[nodiscard]] virtual numerical::Matrix<N, N, T> velocity_p() const = 0;
        [[nodiscard]] virtual numerical::Vector<2 * N, T> position_velocity() const = 0;
        [[nodiscard]] virtual numerical::Matrix<2 * N, 2 * N, T> position_velocity_p() const = 0;
        [[nodiscard]] virtual T speed() const = 0;
        [[nodiscard]] virtual T speed_p() const = 0;

        [[nodiscard]] virtual numerical::Vector<N, T> x_to_position(const numerical::Vector<D, T>& x) const = 0;
        [[nodiscard]] virtual numerical::Vector<N, T> p_to_position_p(const numerical::Matrix<D, D, T>& p) const = 0;
        [[nodiscard]] virtual T x_to_speed(const numerical::Vector<D, T>& x) const = 0;
        [[nodiscard]] virtual T xp_to_speed_p(const numerical::Vector<D, T>& x, const numerical::Matrix<D, D, T>& p)
                const = 0;
};

template <std::size_t N, typename T>
class Filter
{
public:
        virtual ~Filter() = default;

        [[nodiscard]] virtual std::optional<UpdateInfo<N, T>> update(
                const Measurements<N, T>& m,
                const Estimation<N, T>& estimation) = 0;

        [[nodiscard]] virtual std::string consistency_string() const = 0;
};
}
