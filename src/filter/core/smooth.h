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
#include <deque>
#include <tuple>
#include <vector>

namespace ns::filter::core
{
namespace smooth_implementation
{
template <std::size_t N, typename T>
void smooth(
        const numerical::Matrix<N, N, T>& predict_f_next,
        const numerical::Vector<N, T>& predict_x_next,
        const numerical::Matrix<N, N, T>& predict_p_next,
        const numerical::Vector<N, T>& x_next,
        const numerical::Matrix<N, N, T>& p_next,
        numerical::Vector<N, T>& x,
        numerical::Matrix<N, N, T>& p)
{
        const numerical::Matrix<N, N, T> k = p * predict_f_next.transposed() * predict_p_next.inversed();

        x = x + k * (x_next - predict_x_next);
        p = p + k * (p_next - predict_p_next) * k.transposed();

        check_x_p("Smooth", x, p);
}
}

template <std::size_t N, typename T>
[[nodiscard]] std::tuple<std::vector<numerical::Vector<N, T>>, std::vector<numerical::Matrix<N, N, T>>> smooth(
        const std::vector<numerical::Matrix<N, N, T>>& predict_f,
        const std::vector<numerical::Vector<N, T>>& predict_x,
        const std::vector<numerical::Matrix<N, N, T>>& predict_p,
        std::vector<numerical::Vector<N, T>> x,
        std::vector<numerical::Matrix<N, N, T>> p)
{
        namespace impl = smooth_implementation;

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == predict_f.size());
        ASSERT(x.size() == predict_x.size());
        ASSERT(x.size() == predict_p.size());

        for (auto i = std::ssize(x) - 2; i >= 0; --i)
        {
                impl::smooth(predict_f[i + 1], predict_x[i + 1], predict_p[i + 1], x[i + 1], p[i + 1], x[i], p[i]);
        }

        return {std::move(x), std::move(p)};
}

template <std::size_t N, typename T>
[[nodiscard]] std::tuple<numerical::Vector<N, T>, numerical::Matrix<N, N, T>> smooth(
        const std::deque<numerical::Matrix<N, N, T>>& predict_f,
        const std::deque<numerical::Vector<N, T>>& predict_x,
        const std::deque<numerical::Matrix<N, N, T>>& predict_p,
        const std::deque<numerical::Vector<N, T>>& x,
        const std::deque<numerical::Matrix<N, N, T>>& p)
{
        namespace impl = smooth_implementation;

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == predict_f.size());
        ASSERT(x.size() == predict_x.size());
        ASSERT(x.size() == predict_p.size());

        if (x.empty())
        {
                error("No data for smoothing");
        }

        numerical::Vector<N, T> x_s = x.back();
        numerical::Matrix<N, N, T> p_s = p.back();

        for (auto i = std::ssize(x) - 2; i >= 0; --i)
        {
                const auto x_next = x_s;
                const auto p_next = p_s;

                x_s = x[i];
                p_s = p[i];

                impl::smooth(predict_f[i + 1], predict_x[i + 1], predict_p[i + 1], x_next, p_next, x_s, p_s);
        }

        return {x_s, p_s};
}
}
