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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <string>

namespace ns::filter::core::test::filters
{
template <typename T, bool INF>
class FilterEkf
{
public:
        using Type = T;

        virtual ~FilterEkf() = default;

        virtual void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p) = 0;

        virtual void predict(T dt, T process_variance) = 0;

        virtual void update_position(T position, T position_variance) = 0;

        virtual void update_position_speed(T position, T position_variance, T speed, T speed_variance) = 0;

        [[nodiscard]] virtual T position() const = 0;
        [[nodiscard]] virtual T position_p() const = 0;

        [[nodiscard]] virtual numerical::Vector<2, T> position_speed() const = 0;
        [[nodiscard]] virtual numerical::Matrix<2, 2, T> position_speed_p() const = 0;

        [[nodiscard]] virtual std::string name() const = 0;
};

template <typename T, bool INF>
[[nodiscard]] std::unique_ptr<FilterEkf<T, INF>> create_filter_ekf();
}
