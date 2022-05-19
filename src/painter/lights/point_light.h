/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "common.h"

#include "../objects.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <type_traits>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class PointLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> location_;
        Color color_;
        T coef_;

public:
        PointLight(
                const Vector<N, T>& location,
                const Color& color,
                const std::type_identity_t<T>& unit_intensity_distance)
                : location_(location),
                  color_(color),
                  coef_(power<N - 1>(unit_intensity_distance))
        {
                if (!(unit_intensity_distance > 0))
                {
                        error("Error unit intensity distance " + to_string(unit_intensity_distance));
                }
        }

        [[nodiscard]] LightSourceSample<N, T, Color> sample(PCG& /*engine*/, const Vector<N, T>& point) const override
        {
                const Vector<N, T> direction = location_ - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);
                const T coef = coef_ / lights::common::power_n1<N>(squared_distance, distance);

                LightSourceSample<N, T, Color> s;
                s.distance = distance;
                s.l = direction / distance;
                s.pdf = 1;
                s.radiance = color_ * coef;
                return s;
        }

        [[nodiscard]] LightSourceInfo<T, Color> info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/)
                const override
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

        [[nodiscard]] bool is_delta() const override
        {
                return true;
        }
};
}
