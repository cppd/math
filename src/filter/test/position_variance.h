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
template <typename T>
class PositionVariance final
{
        static constexpr std::size_t VARIANCE_WINDOW_SIZE{500};
        static constexpr unsigned VARIANCE_ESTIMATION_COUNT{80};
        static constexpr unsigned VARIANCE_ESTIMATION_FILTER_COUNT{15};
        static constexpr Vector<2, T> VARIANCE_DEFAULT{square(T{1})};
        static constexpr T VARIANCE_MIN{square(T{0.1L})};
        static constexpr T VARIANCE_MAX{square(T{500})};

        std::optional<std::array<std::vector<T>, 2>> estimation_residuals_;
        numerical::MovingVariance<Vector<2, T>> variance_;

        void fill_estimation(const Vector<2, T>& residual)
        {
                static constexpr std::size_t FILTER = VARIANCE_ESTIMATION_FILTER_COUNT;
                static constexpr std::size_t SIZE = VARIANCE_ESTIMATION_COUNT + 2 * FILTER;

                ASSERT(estimation_residuals_);

                std::array<std::vector<T>, 2>& r = *estimation_residuals_;

                ASSERT(r[0].size() == r[1].size());
                ASSERT(r[0].size() < SIZE);

                r[0].push_back(residual[0]);
                r[1].push_back(residual[1]);

                ASSERT(r[0].size() == r[1].size());
                if (r[0].size() < SIZE)
                {
                        return;
                }

                std::ranges::sort(r[0]);
                std::ranges::sort(r[1]);

                ASSERT(r[0].size() == SIZE);
                ASSERT(r[1].size() == SIZE);
                static_assert(SIZE > 2 * FILTER);
                for (std::size_t i = FILTER; i < SIZE - FILTER; ++i)
                {
                        variance_.push({r[0][i], r[1][i]});
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

        void push(const Vector<2, T>& residual)
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

        [[nodiscard]] std::optional<Vector<2, T>> mean() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());
                return variance_.mean();
        }

        [[nodiscard]] std::optional<Vector<2, T>> standard_deviation() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());
                return variance_.standard_deviation();
        }

        [[nodiscard]] std::optional<Vector<2, T>> compute() const
        {
                if (!has_variance())
                {
                        return {};
                }

                ASSERT(variance_.has_variance());
                const Vector<2, T> v2 = variance_.variance();
                const T sd = (std::sqrt(v2[0]) + std::sqrt(v2[1])) / 2;
                const T v1 = std::clamp<T>(square(sd), VARIANCE_MIN, VARIANCE_MAX);
                return Vector<2, T>(v1);
        }

        [[nodiscard]] static const Vector<2, T>& default_variance()
        {
                return VARIANCE_DEFAULT;
        }
};
}