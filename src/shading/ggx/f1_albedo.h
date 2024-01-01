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

#include <array>
#include <cstddef>
#include <span>
#include <tuple>

namespace ns::shading::ggx
{
template <std::size_t N, typename T>
T f1_albedo(T roughness, T cosine);

template <std::size_t N, typename T>
T f1_albedo_cosine_weighted_average(T roughness);

template <std::size_t N, typename T>
std::tuple<std::array<int, 2>, std::span<const T>> f1_albedo_cosine_roughness_data();

template <std::size_t N, typename T>
std::tuple<std::array<int, 1>, std::span<const T>> f1_albedo_cosine_weighted_average_data();
}
