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

#include "estimation.h"
#include "measurement.h"

#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>
#include <string>

namespace ns::filter::filters
{
template <std::size_t N, typename T>
struct UpdateInfo final
{
        Vector<N, T> position;
        Vector<N, T> position_p;
        T speed;
        T speed_p;
};

template <std::size_t N, typename T>
class FilterPosition
{
public:
        virtual ~FilterPosition() = default;

        [[nodiscard]] virtual std::optional<UpdateInfo<N, T>> update(const Measurements<N, T>& m) = 0;
        [[nodiscard]] virtual std::optional<UpdateInfo<N, T>> predict(const Measurements<N, T>& m) = 0;

        [[nodiscard]] virtual std::string consistency_string() const = 0;
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
