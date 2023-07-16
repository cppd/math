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

#include <cmath>
#include <optional>

namespace ns::filter::test
{
template <typename T>
class PositionVariance final
{
        static constexpr std::size_t VARIANCE_WINDOW_SIZE = 500;
        static constexpr unsigned VARIANCE_ESTIMATION_COUNT = 50;
        static constexpr Vector<2, T> VARIANCE_DEFAULT{square(T{30})};
        static constexpr T VARIANCE_MIN = square(T{0.1L});
        static constexpr T VARIANCE_MAX = square(T{1000});

        numerical::MovingVariance<Vector<2, T>> variance_;

public:
        PositionVariance()
                : variance_{VARIANCE_WINDOW_SIZE}
        {
        }

        void push(const Vector<2, T>& residual)
        {
                variance_.push(residual);
        }

        [[nodiscard]] const numerical::MovingVariance<Vector<2, T>>& variance() const
        {
                return variance_;
        }

        [[nodiscard]] std::optional<Vector<2, T>> compute() const
        {
                if (variance_.size() < VARIANCE_ESTIMATION_COUNT)
                {
                        return {};
                }

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
