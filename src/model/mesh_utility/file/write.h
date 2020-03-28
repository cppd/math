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

#include <src/numerical/vec.h>
#include <src/utility/file/file.h>

namespace mesh::file
{
template <size_t N, typename T>
void write_vector(const CFile& file, const Vector<N, T>& vector)
{
        static_assert(limits<float>::max_digits10 <= 9);
        static_assert(limits<double>::max_digits10 <= 17);
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        constexpr const char* format = (std::is_same_v<T, float>) ? " %16.9e" : " %24.17e";

        for (unsigned i = 0; i < N; ++i)
        {
                fprintf(file, format, vector[i]);
        }
}
}
