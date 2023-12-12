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

#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::filters
{
template <std::size_t N, typename T>
struct TrueData final
{
        Vector<N, T> position;
        T speed;
        T angle;
        T angle_r;
};

template <std::size_t N, typename T>
struct PositionMeasurement final
{
        Vector<N, T> value;
        std::optional<Vector<N, T>> variance;
};

template <std::size_t N, typename T>
struct Measurement final
{
        Vector<N, T> value;
        Vector<N, T> variance;
};

template <std::size_t N, typename T>
struct Measurements final
{
        TrueData<N, T> true_data;
        T time;
        std::optional<PositionMeasurement<N, T>> position;
        std::optional<Measurement<N, T>> acceleration;
        std::optional<Measurement<1, T>> direction;
        std::optional<Measurement<1, T>> speed;
};
}
