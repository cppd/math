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

#include "allan_deviation.h"

#include <vector>

namespace ns::filter::analysis
{
template <typename T>
struct NoiseParameterLine final
{
        T tau;
        T deviation;
        T log_slope;
};

template <typename T>
struct NoiseParameter final
{
        T value;
        NoiseParameterLine<T> line;
};

template <typename T>
[[nodiscard]] NoiseParameter<T> bias_instability(const std::vector<AllanDeviation<T>>& allan_deviation);

template <typename T>
[[nodiscard]] NoiseParameter<T> angle_random_walk(const std::vector<AllanDeviation<T>>& allan_deviation);

template <typename T>
[[nodiscard]] NoiseParameter<T> rate_random_walk(const std::vector<AllanDeviation<T>>& allan_deviation);
}
