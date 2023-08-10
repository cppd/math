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

#include <array>
#include <optional>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
class MovingVariance final
{
        static_assert(N > 0);

        static constexpr std::size_t VARIANCE_WINDOW_SIZE{500};
        static constexpr unsigned VARIANCE_MIN_COUNT{80};

        std::optional<std::array<std::vector<T>, N>> estimation_residuals_;
        numerical::MovingVariance<Vector<N, T>> variance_;

        void fill_estimation(const Vector<N, T>& residual);

public:
        MovingVariance()
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
                return variance_.size() >= VARIANCE_MIN_COUNT;
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

        [[nodiscard]] std::optional<Vector<N, T>> compute() const;
};
}
