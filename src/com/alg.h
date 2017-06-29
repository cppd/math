/*
Copyright (C) 2017 Topological Manifold

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

#include <algorithm>

template <typename T>
void sort_and_unique(T* v)
{
        std::sort(v->begin(), v->end());
        v->erase(std::unique(v->begin(), v->end()), v->end());
}

// Можно вместо этого использовать std::all_of или std::find(v.cbegin(), v.cend(), true) == v.cend()
template <typename T>
bool all_false(const T& v)
{
        int size = v.size();
        for (int i = 0; i < size; ++i)
        {
                if (v[i])
                {
                        return false;
                }
        }
        return true;
}
// Можно вместо этого использовать std::all_of или std::find(v.cbegin(), v.cend(), false) == v.cend()
template <typename T>
bool all_true(const T& v)
{
        int size = v.size();
        for (int i = 0; i < size; ++i)
        {
                if (!v[i])
                {
                        return false;
                }
        }
        return true;
}
