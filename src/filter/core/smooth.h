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

#include "checks.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::filter::core
{
template <std::size_t N, typename T>
[[nodiscard]] std::vector<numerical::Vector<N, T>> smooth(
        std::vector<numerical::Vector<N, T>> x,
        const std::vector<numerical::Matrix<N, N, T>>& p,
        const numerical::Matrix<N, N, T>& f,
        const numerical::Matrix<N, N, T>& q)
{
        ASSERT(x.size() == p.size());

        if (x.size() <= 1)
        {
                return x;
        }

        numerical::Matrix<N, N, T> p_next = p.back();

        for (auto i = std::ssize(x) - 2; i >= 0; --i)
        {
                const numerical::Matrix<N, N, T> ft = f.transposed();
                const numerical::Matrix<N, N, T> pp = f * p[i] * ft + q;
                const numerical::Matrix<N, N, T> k = p[i] * ft * pp.inversed();

                x[i] = x[i] + k * (x[i + 1] - f * x[i]);
                p_next = p[i] + k * (p_next - pp) * k.transposed();

                check_x_p("Smooth", x[i], p_next);
        }

        return x;
}
}
