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

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/geometry/shapes/sphere_area.h>

#include <cmath>

namespace ns::painter::lights::com
{
namespace spot_light_implementation
{
template <std::size_t N, typename T>
T cone_solid_angle(const std::type_identity_t<T> angle)
{
        const T ratio =
                geometry::sphere_relative_area<N, T>(0, angle) / geometry::sphere_relative_area<N, T>(0, PI<T> / 2);
        return (geometry::SPHERE_AREA<N, T> / 2) * ratio;
}

template <std::size_t N, typename T>
T cone_solid_angle(const std::type_identity_t<T> start_degrees, const std::type_identity_t<T> width_degrees)
{
        return cone_solid_angle<N, T>(degrees_to_radians((start_degrees + width_degrees) / 2));
}
}

template <std::size_t N, typename T>
class SpotLight final
{
        static_assert(std::is_floating_point_v<T>);

        T falloff_start_;
        T width_;
        T falloff_width_;
        T cone_solid_angle_;

        [[nodiscard]] T falloff_coef(const T cosine) const
        {
                return power<4>((cosine - width_) / falloff_width_);
        }

public:
        SpotLight(const std::type_identity_t<T> falloff_start_degrees, const std::type_identity_t<T> width_degrees)
                : falloff_start_(std::cos(degrees_to_radians(falloff_start_degrees))),
                  width_(std::cos(degrees_to_radians(width_degrees))),
                  falloff_width_(falloff_start_ - width_),
                  cone_solid_angle_(
                          spot_light_implementation::cone_solid_angle<N, T>(falloff_start_degrees, width_degrees))
        {
                if (!(falloff_start_degrees >= 0 && width_degrees > 0 && falloff_start_degrees <= width_degrees
                      && width_degrees <= 180))
                {
                        error("Error falloff start " + to_string(falloff_start_degrees) + " and width "
                              + to_string(width_degrees));
                }

                ASSERT((falloff_start_degrees == width_degrees) == (falloff_start_ == width_));
                ASSERT(falloff_start_ >= width_ && falloff_width_ >= 0);
        }

        [[nodiscard]] T coef(const T cosine) const
        {
                if (cosine >= falloff_start_)
                {
                        return 1;
                }
                if (cosine <= width_)
                {
                        return 0;
                }
                return falloff_coef(cosine);
        }

        template <typename Color>
        [[nodiscard]] Color color(const Color& color, const T cosine) const
        {
                if (cosine >= falloff_start_)
                {
                        return color;
                }
                if (cosine <= width_)
                {
                        return Color(0);
                }
                return color * falloff_coef(cosine);
        }

        T solid_angle() const
        {
                return cone_solid_angle_;
        }
};
}
