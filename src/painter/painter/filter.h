/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/numerical/vec.h>

#include <cmath>

namespace ns::painter
{
template <typename T>
class GaussianFilter final
{
        static T gaussian(T negative_alpha, T v)
        {
                return std::exp(negative_alpha * v * v);
        }

        T m_negative_alpha;
        T m_exp;

public:
        GaussianFilter(std::type_identity_t<T> alpha, std::type_identity_t<T> radius)
        {
                if (!(alpha > 0))
                {
                        error("Gaussian alpha " + to_string(alpha) + " must be positive");
                }

                if (!(radius > 0))
                {
                        error("Gaussian radius " + to_string(radius) + " must be positive");
                }

                m_negative_alpha = -alpha;
                m_exp = gaussian(m_negative_alpha, radius);
        }

        template <std::size_t N>
        T compute(const Vector<N, T>& p) const
        {
                static_assert(N >= 1);

                T r = std::max(T(0), gaussian(m_negative_alpha, p[0]) - m_exp);
                for (std::size_t i = 1; i < N; ++i)
                {
                        r *= std::max(T(0), gaussian(m_negative_alpha, p[i]) - m_exp);
                }
                return r;
        }
};
}
