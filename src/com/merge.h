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

#include <iterator>
#include <utility>

namespace ns
{
namespace merge_implementation
{
template <typename T1, typename T2>
concept VectorAndContainer =
        requires (T1* const v1, const T2& v2) {
                v1->push_back(*std::begin(v2));
                v1->insert(v1->end(), std::begin(v2), std::end(v2));
        };

template <typename T1, typename T2>
concept SetAndContainer =
        requires (T1* const v1, const T2& v2) {
                v1->contains(*std::begin(v2));
                v1->insert(std::begin(v2), std::end(v2));
        };

template <typename T1, typename T2>
concept VectorAndValue = requires (T1* const v1, T2&& v2) { v1->push_back(std::forward<T2>(v2)); };

template <typename T1, typename T2>
concept SetAndValue =
        requires (T1* const v1, T2&& v2) {
                v1->contains(v2);
                v1->insert(std::forward<T2>(v2));
        };

//

template <typename T1, typename T2>
        requires VectorAndContainer<T1, T2>
void merge(T1* const v1, const T2& v2)
{
        v1->insert(v1->end(), std::begin(v2), std::end(v2));
}

template <typename T1, typename T2>
        requires SetAndContainer<T1, T2>
void merge(T1* const v1, const T2& v2)
{
        v1->insert(std::begin(v2), std::end(v2));
}

template <typename T1, typename T2>
        requires VectorAndValue<T1, T2>
void merge(T1* const v1, T2&& v2)
{
        v1->push_back(std::forward<T2>(v2));
}

template <typename T1, typename T2>
        requires SetAndValue<T1, T2>
void merge(T1* const v1, T2&& v2)
{
        v1->insert(std::forward<T2>(v2));
}
}

template <typename Result, typename... T>
Result merge(T&&... data)
{
        Result res;
        (merge_implementation::merge(&res, std::forward<T>(data)), ...);
        return res;
}
}
