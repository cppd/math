/*
Copyright (C) 2017-2024 Topological Manifold

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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

7.8.1 Filter functions
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <type_traits>

namespace ns::painter::pixels
{
template <typename T>
class Gaussian final
{
        [[nodiscard]] static T gaussian(const T alpha, const T v)
        {
                return std::exp(alpha * v * v);
        }

        T alpha_;
        T exp_;

public:
        Gaussian(const std::type_identity_t<T> width, const std::type_identity_t<T> radius)
        {
                if (!(width > 0))
                {
                        error("Gaussian filter width " + to_string(width) + " must be positive");
                }

                if (!(radius > 0))
                {
                        error("Gaussian filter radius " + to_string(radius) + " must be positive");
                }

                alpha_ = -1 / (2 * width * width);
                exp_ = gaussian(alpha_, radius);
        }

        template <std::size_t N>
        [[nodiscard]] T compute(const Vector<N, T>& p) const
        {
                static_assert(N >= 1);

                T res = std::max(T{0}, gaussian(alpha_, p[0]) - exp_);
                for (std::size_t i = 1; i < N; ++i)
                {
                        res *= std::max(T{0}, gaussian(alpha_, p[i]) - exp_);
                }
                return res;
        }
};
}
