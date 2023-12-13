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

#include "gaussian.h"

#include <src/com/error.h>
#include <src/com/math.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ns::painter::pixels
{
template <std::size_t N, typename T>
class PixelFilter final
{
        // radius=1.5;
        // width=radius/2.5;
        // alpha=1/(2*width*width);
        // gaussian[x_]:=Exp[-alpha*x*x];
        // gaussianFilter[x_]:=gaussian[x]-gaussian[radius];
        // max=gaussianFilter[0];
        // triangle[x_]:=If[x<0,max/radius*x+max,-max/radius*x+max];
        // Plot[{gaussianFilter[x],triangle[x]},{x,-radius,radius},PlotRange->Full]
        static constexpr T FILTER_RADIUS = 1.5;
        static constexpr T GAUSSIAN_FILTER_WIDTH = FILTER_RADIUS / 2.5;

        static constexpr int INTEGER_RADIUS = integral_ceil<int>(std::max(T{0}, FILTER_RADIUS - T{0.5}));
        static_assert(INTEGER_RADIUS == 1);

        const Gaussian<T> filter_{GAUSSIAN_FILTER_WIDTH, FILTER_RADIUS};

public:
        [[nodiscard]] static constexpr int integer_radius()
        {
                return INTEGER_RADIUS;
        }

        void compute_weights(
                const Vector<N, T>& center,
                const std::vector<Vector<N, T>>& points,
                std::vector<T>* const weights) const
        {
                weights->resize(points.size());
                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        const T weight = filter_.compute(center - points[i]);
                        ASSERT(weight >= 0);
                        (*weights)[i] = weight;
                }
        }
};
}
