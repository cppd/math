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

#pragma once

#include "../objects.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <type_traits>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class SpotLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // power<N - 1>(distance)
        static T power_n1(const T& squared_distance, const T& distance)
        {
                if constexpr ((N & 1) == 1)
                {
                        return power<((N - 1) / 2)>(squared_distance);
                }
                else
                {
                        return power<((N - 2) / 2)>(squared_distance) * distance;
                }
        }

        Vector<N, T> location_;
        Vector<N, T> direction_;
        Color color_;
        T coef_;
        T falloff_start_;
        T width_;
        T falloff_width_;

public:
        SpotLight(
                const Vector<N, T>& location,
                const Vector<N, T>& direction,
                const Color& color,
                std::type_identity_t<T> unit_intensity_distance,
                std::type_identity_t<T> falloff_start,
                std::type_identity_t<T> width)
                : location_(location),
                  direction_(direction.normalized()),
                  color_(color),
                  coef_(power<N - 1>(unit_intensity_distance)),
                  falloff_start_(std::cos(falloff_start * (PI<T> / 180))),
                  width_(std::cos(width * (PI<T> / 180))),
                  falloff_width_(falloff_start_ - width_)
        {
                if (!(unit_intensity_distance > 0))
                {
                        error("Error unit intensity distance " + to_string(unit_intensity_distance));
                }

                if (!(falloff_start >= 0 && width > 0 && falloff_start <= width && width <= 180))
                {
                        error("Error falloff start " + to_string(falloff_start) + " and width " + to_string(width));
                }

                ASSERT(falloff_start_ >= width_ && falloff_width_ >= 0);
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& /*random_engine*/, const Vector<N, T>& point)
                const override
        {
                const Vector<N, T> direction = location_ - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);
                const Vector<N, T> l = direction / distance;
                const T cos = -dot(l, direction_);

                LightSourceSample<N, T, Color> s;

                s.distance = distance;
                s.l = l;
                s.pdf = 1;

                if (cos <= width_)
                {
                        s.radiance = Color(0);
                        return s;
                }

                const T coef = coef_ / power_n1(squared_distance, distance);
                if (cos >= falloff_start_)
                {
                        s.radiance = color_ * coef;
                }
                else
                {
                        T k = power<4>((cos - width_) / falloff_width_);
                        s.radiance = color_ * (coef * k);
                }
                return s;
        }

        LightSourceInfo<T, Color> info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const override
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

        bool is_delta() const override
        {
                return true;
        }
};
}
