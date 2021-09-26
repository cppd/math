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
namespace point_light_implementation
{
template <std::size_t N, typename T>
T compute_coef(const T& coef, const T& squared_distance, const T& distance)
{
        if constexpr ((N & 1) == 1)
        {
                return coef / power<((N - 1) / 2)>(squared_distance);
        }
        else
        {
                return coef / power<((N - 2) / 2)>(squared_distance) * distance;
        }
}
}

template <std::size_t N, typename T, typename Color>
class PointLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> location_;
        Color color_;
        T coef_;

public:
        PointLight(const Vector<N, T>& location, const Color& color, std::type_identity_t<T> unit_intensity_distance)
                : location_(location), color_(color), coef_(std::pow(unit_intensity_distance, T(N - 1)))
        {
                if (!(unit_intensity_distance > 0))
                {
                        error("Error unit intensity distance " + to_string(unit_intensity_distance));
                }
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& /*random_engine*/, const Vector<N, T>& point)
                const override
        {
                namespace impl = point_light_implementation;

                const Vector<N, T> direction = location_ - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);
                const T coef = impl::compute_coef<N>(coef_, squared_distance, distance);

                LightSourceSample<N, T, Color> s;
                s.distance = distance;
                s.l = direction / distance;
                s.pdf = 1;
                s.radiance = color_ * coef;
                return s;
        }

        T pdf(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const override
        {
                return 0;
        }

        bool is_delta() const override
        {
                return true;
        }
};

template <std::size_t N, typename T, typename Color>
class SpotLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

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
                  coef_(std::pow(unit_intensity_distance, T(N - 1))),
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
                namespace impl = point_light_implementation;

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

                const T coef = impl::compute_coef<N>(coef_, squared_distance, distance);
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

        T pdf(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const override
        {
                return 0;
        }

        bool is_delta() const override
        {
                return true;
        }
};
}
