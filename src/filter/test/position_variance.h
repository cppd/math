/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/numerical/variance.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace ns::filter::test
{
template <std::size_t N, typename T>
class PositionVariance final
{
        static_assert(N > 0);

        static constexpr std::size_t VARIANCE_WINDOW_SIZE{500};
        static constexpr unsigned VARIANCE_ESTIMATION_COUNT{80};
        static constexpr unsigned VARIANCE_ESTIMATION_FILTER_COUNT{15};
        static constexpr Vector<N, T> VARIANCE_DEFAULT{square(T{1})};
        static constexpr T VARIANCE_MIN{square(T{0.1L})};
        static constexpr T VARIANCE_MAX{square(T{500})};

        std::optional<std::array<std::vector<T>, N>> estimation_residuals_;
        numerical::MovingVariance<Vector<N, T>> variance_;

        void fill_estimation(const Vector<N, T>& residual)
        {
                static constexpr std::size_t FILTER = VARIANCE_ESTIMATION_FILTER_COUNT;
                static constexpr std::size_t SIZE = VARIANCE_ESTIMATION_COUNT + 2 * FILTER;

                ASSERT(estimation_residuals_);

                std::array<std::vector<T>, N>& r = *estimation_residuals_;

                ASSERT(std::ranges::all_of(
                        r,
                        [&](const std::vector<T>& v)
                        {
                                return v.size() == r[0].size() && v.size() < SIZE;
                        }));

                for (std::size_t i = 0; i < N; ++i)
                {
                        r[i].push_back(residual[i]);
                }

                if (r[0].size() < SIZE)
                {
                        return;
                }

                for (std::size_t i = 0; i < N; ++i)
                {
                        std::ranges::sort(r[i]);
                }

                ASSERT(std::ranges::all_of(
                        r,
                        [&](const std::vector<T>& v)
                        {
                                return v.size() == SIZE;
                        }));

                static_assert(SIZE > 2 * FILTER);
                for (std::size_t i = FILTER; i < SIZE - FILTER; ++i)
                {
                        Vector<N, T> v;
                        for (std::size_t n = 0; n < N; ++n)
                        {
                                v[n] = r[n][i];
                        }
                        variance_.push(v);
                }

                ASSERT(variance_.size() == VARIANCE_ESTIMATION_COUNT);
                estimation_residuals_.reset();
        }

public:
        PositionVariance()
                : variance_{VARIANCE_WINDOW_SIZE}
        {
                estimation_residuals_.emplace();
        }

        void push(const Vector<N, T>& residual)
        {
                if (estimation_residuals_)
                {
                        fill_estimation(residual);
                        return;
                }
                variance_.push(residual);
        }

        [[nodiscard]] bool has_variance() const
        {
                return variance_.size() >= VARIANCE_ESTIMATION_COUNT;
        }

        [[nodiscard]] std::optional<Vector<N, T>> mean() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());
                return variance_.mean();
        }

        [[nodiscard]] std::optional<Vector<N, T>> standard_deviation() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());
                return variance_.standard_deviation();
        }

        [[nodiscard]] std::optional<Vector<N, T>> compute() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());

                const Vector<N, T> v = variance_.variance();
                T sum = 0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        sum += std::sqrt(v[i]);
                }
                const T sd = sum / N;
                return Vector<N, T>(std::clamp<T>(square(sd), VARIANCE_MIN, VARIANCE_MAX));
        }

        [[nodiscard]] static const Vector<N, T>& default_variance()
        {
                return VARIANCE_DEFAULT;
        }
};
}
