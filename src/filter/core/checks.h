/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <string>

namespace ns::filter::core
{
template <std::size_t N, typename T>
[[nodiscard]] bool positive_definite(const numerical::Matrix<N, N, T>& p)
{
        // this is insufficient check based on diagonal only
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(p[i, i] > 0))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
void check_x_p(const char* const name, const numerical::Vector<N, T>& x, const numerical::Matrix<N, N, T>& p)
{
        if (!positive_definite(p))
        {
                error(std::string(name) + ", diagonal is not positive\nx\n" + to_string(x) + "\np\n" + to_string(p));
        }
}
}
