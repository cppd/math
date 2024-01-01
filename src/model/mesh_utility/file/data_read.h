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

#include <src/com/error.h>
#include <src/com/read.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::model::mesh::file
{
template <typename Iter, typename Op>
[[nodiscard]] Iter read(Iter first, const Iter last, const Op& op)
{
        while (first < last && op(*first))
        {
                ++first;
        }
        return first;
}

template <std::size_t N, typename T>
const char* read(const char* str, Vector<N, T>* const v)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto [value, ptr] = read_from_chars<T>(str);
                if (value)
                {
                        (*v)[i] = *value;
                        str = ptr;
                        continue;
                }
                error("Error reading vector data");
        }

        return str;
}

template <std::size_t N, typename T>
const char* read(const char* str, Vector<N, T>* const v, std::optional<T>* const n)
{
        str = read(str, v);

        std::tie(*n, str) = read_from_chars<T>(str);

        return str;
}
}
