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
#include <tuple>
#include <vector>

namespace ns::filter::core
{
template <std::size_t N, typename T>
[[nodiscard]] std::tuple<std::vector<numerical::Vector<N, T>>, std::vector<numerical::Matrix<N, N, T>>> smooth(
        const std::vector<numerical::Matrix<N, N, T>>& f_predict,
        const std::vector<numerical::Vector<N, T>> x_predict,
        const std::vector<numerical::Matrix<N, N, T>>& p_predict,
        std::vector<numerical::Vector<N, T>> x,
        std::vector<numerical::Matrix<N, N, T>> p)
{
        ASSERT(x.size() == p.size());
        ASSERT(x.size() == x_predict.size());
        ASSERT(x.size() == p_predict.size());
        ASSERT(x.size() == f_predict.size());

        if (x.size() <= 1)
        {
                return {x, p};
        }

        for (auto i = std::ssize(x) - 2; i >= 0; --i)
        {
                const numerical::Matrix<N, N, T> k = p[i] * f_predict[i + 1].transposed() * p_predict[i + 1].inversed();

                x[i] = x[i] + k * (x[i + 1] - x_predict[i + 1]);
                p[i] = p[i] + k * (p[i + 1] - p_predict[i + 1]) * k.transposed();

                check_x_p("Smooth", x[i], p[i]);
        }

        return {std::move(x), std::move(p)};
}
}
