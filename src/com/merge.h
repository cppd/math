/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "alg.h"

#include "type/detect.h"

#include <vector>

namespace ns
{
namespace merge_implementation
{
template <typename T1, typename T2>
void add(std::vector<T1>* v, const T2& e)
{
        if constexpr (is_vector<T2> || is_array<T2>)
        {
                v->insert(v->end(), e.cbegin(), e.cend());
        }
        else
        {
                v->insert(v->end(), e);
        }
}
}

template <typename R, typename... T>
std::vector<R> merge(const T&... data)
{
        std::vector<R> res;
        (merge_implementation::add(&res, data), ...);
        return unique_elements(std::move(res));
}
}
