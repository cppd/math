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
#include <type_traits>
#include <vector>

namespace ns::color::samples
{
template <typename ResultType, typename T>
std::vector<ResultType> average(
        std::span<const T> waves,
        std::span<const T> samples,
        std::type_identity_t<T> from,
        std::type_identity_t<T> to,
        std::size_t count);

template <typename ResultType, typename T, std::size_t N>
std::vector<ResultType> average(
        const std::array<T, N>& waves,
        const std::array<T, N>& samples,
        const std::type_identity_t<T> from,
        const std::type_identity_t<T> to,
        const std::size_t count)
{
        return average<ResultType>(std::span<const T>(waves), std::span<const T>(samples), from, to, count);
}

template <typename ResultType, typename T>
std::vector<ResultType> average(
        const std::vector<T>& waves,
        const std::vector<T>& samples,
        const std::type_identity_t<T> from,
        const std::type_identity_t<T> to,
        const std::size_t count)
{
        return average<ResultType>(std::span<const T>(waves), std::span<const T>(samples), from, to, count);
}
}
