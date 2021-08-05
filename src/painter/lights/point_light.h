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
#include <src/com/math.h>
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

        Vector<N, T> m_location;
        Color m_color;
        T m_coef;

public:
        PointLight(const Vector<N, T>& location, const Color& color, std::type_identity_t<T> unit_intensity_distance)
                : m_location(location), m_color(color), m_coef(std::pow(unit_intensity_distance, T(N - 1)))
        {
                if (!(unit_intensity_distance > 0))
                {
                        error("Error unit intensity distance " + to_string(unit_intensity_distance));
                }
        }

        LightSourceSample<N, T, Color> sample(const Vector<N, T>& point) const override
        {
                namespace impl = point_light_implementation;

                const Vector<N, T> direction = m_location - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);
                const T coef = impl::compute_coef<N>(m_coef, squared_distance, distance);

                LightSourceSample<N, T, Color> s;
                s.distance = distance;
                s.l = direction / distance;
                s.pdf = 1;
                s.L = m_color * coef;
                return s;
        }
};

template <std::size_t N, typename T, typename Color>
class SpotLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> m_location;
        Vector<N, T> m_direction;
        Color m_color;
        T m_coef;
        T m_falloff_start;
        T m_width;
        T m_falloff_width;

public:
        SpotLight(
                const Vector<N, T>& location,
                const Vector<N, T>& direction,
                const Color& color,
                std::type_identity_t<T> unit_intensity_distance,
                std::type_identity_t<T> falloff_start,
                std::type_identity_t<T> width)
                : m_location(location),
                  m_direction(direction.normalized()),
                  m_color(color),
                  m_coef(std::pow(unit_intensity_distance, T(N - 1))),
                  m_falloff_start(std::cos(falloff_start * (PI<T> / 180))),
                  m_width(std::cos(width * (PI<T> / 180))),
                  m_falloff_width(m_falloff_start - m_width)
        {
                if (!(unit_intensity_distance > 0))
                {
                        error("Error unit intensity distance " + to_string(unit_intensity_distance));
                }

                if (!(falloff_start >= 0 && width > 0 && falloff_start <= width && width <= 180))
                {
                        error("Error falloff start " + to_string(falloff_start) + " and width " + to_string(width));
                }

                ASSERT(m_falloff_start >= m_width && m_falloff_width >= 0);
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& /*random_engine*/, const Vector<N, T>& point)
                const override
        {
                namespace impl = point_light_implementation;

                const Vector<N, T> direction = m_location - point;
                const T squared_distance = direction.norm_squared();
                const T distance = std::sqrt(squared_distance);
                const Vector<N, T> l = direction / distance;
                const T cos = -dot(l, m_direction);

                LightSourceSample<N, T, Color> s;

                s.distance = distance;
                s.l = l;
                s.pdf = 1;

                if (cos <= m_width)
                {
                        s.L = Color(0);
                        return s;
                }

                const T coef = impl::compute_coef<N>(m_coef, squared_distance, distance);
                if (cos >= m_falloff_start)
                {
                        s.L = m_color * coef;
                }
                else
                {
                        T k = power<4>((cos - m_width) / m_falloff_width);
                        s.L = m_color * (coef * k);
                }
                return s;
        }

        T pdf(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const override
        {
                return 0;
        }
};
}
