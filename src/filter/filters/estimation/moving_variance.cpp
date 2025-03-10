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

#include "moving_variance.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::filter::filters::estimation
{
namespace
{
template <typename T>
constexpr T VARIANCE_MIN{square(T{0.1L})};

template <typename T>
constexpr T VARIANCE_MAX{square(T{500})};

constexpr std::size_t WINDOW_SIZE{500};
constexpr std::size_t FILTER_WINDOW_SIZE{50};
constexpr std::size_t FILTER_SIZE{5};
}

template <std::size_t N, typename T>
MovingVariance<N, T>::MovingVariance()
        : variance_{WINDOW_SIZE}
{
}

template <std::size_t N, typename T>
void MovingVariance<N, T>::push(const numerical::Vector<N, T>& residual)
{
        static constexpr std::size_t SIZE = FILTER_WINDOW_SIZE + 2 * FILTER_SIZE;

        std::array<std::vector<T>, N>& r = estimation_residuals_;

        ASSERT(std::ranges::all_of(
                r,
                [&](const std::vector<T>& v)
                {
                        return v.size() == r.front().size() && v.size() < SIZE;
                }));

        for (std::size_t i = 0; i < N; ++i)
        {
                r[i].push_back(residual[i]);
        }

        if (r.front().size() < SIZE)
        {
                return;
        }

        for (std::vector<T>& v : r)
        {
                std::ranges::sort(v);
        }

        ASSERT(std::ranges::all_of(
                r,
                [&](const std::vector<T>& v)
                {
                        return v.size() == SIZE;
                }));

        static_assert(SIZE > 2 * FILTER_SIZE);
        for (std::size_t i = FILTER_SIZE; i < SIZE - FILTER_SIZE; ++i)
        {
                numerical::Vector<N, T> v;
                for (std::size_t n = 0; n < N; ++n)
                {
                        v[n] = r[n][i];
                }
                variance_.push(v);
        }

        for (std::vector<T>& v : r)
        {
                v.clear();
        }
}

template <std::size_t N, typename T>
std::optional<numerical::Vector<N, T>> MovingVariance<N, T>::compute() const
{
        if (!has_variance())
        {
                return {};
        }

        ASSERT(variance_.has_variance());

        const numerical::Vector<N, T> v = variance_.variance();

        T sum = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
                sum += std::sqrt(v[i]);
        }

        return numerical::Vector<N, T>(std::clamp<T>(square(sum / N), VARIANCE_MIN<T>, VARIANCE_MAX<T>));
}

#define TEMPLATE(N, T) template class MovingVariance<(N), T>;

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
