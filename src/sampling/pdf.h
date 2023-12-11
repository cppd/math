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

#include <src/com/exponent.h>

#include <cstddef>
#include <type_traits>

namespace ns::sampling
{
template <std::size_t N, typename T>
T reflected_pdf(const T pdf, const T cosine)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // reflected_pdf<2> * pow(sin(angle) / sin(reflected), N - 2)
        //   reflected_pdf<2> = pdf / 2
        //   cos(angle) = cosine
        //   cos(reflected) = 2 * square(cosine) - 1
        //   sin(angle) = sqrt(1 - square(cosine))
        //   sin(reflected) = sqrt(1 - square(2 * square(cosine) - 1))
        //   sin(reflected) = sqrt(4 * square(cosine) * (1 - square(cosine)))
        //   sin_ratio = 1 / sqrt(4 * square(cosine)) = 1 / (2 * cosine)
        // pdf / (2 * pow(2 * cosine, N - 2))
        // pdf / (pow(2, N - 1) * pow(cosine, N - 2))

        if (cosine > 0)
        {
                return pdf / ((1 << (N - 1)) * power<N - 2>(cosine));
        }
        return 0;
}

template <std::size_t N, typename T>
T area_pdf_to_solid_angle_pdf(const T pdf, const T cosine, const T distance)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        if (cosine > 0)
        {
                return pdf * power<N - 1>(distance) / cosine;
        }
        return 0;
}

template <std::size_t N, typename T>
T solid_angle_pdf_to_area_pdf(const T pdf, const T cosine, const T distance)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        if (cosine > 0)
        {
                return pdf * cosine / power<N - 1>(distance);
        }
        return 0;
}
}
