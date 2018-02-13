/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/objects.h"

class PointLight final : public LightSource
{
        vec3 m_location;
        Color m_color;
        double m_unit_intensity_distance_square;

public:
        PointLight(const vec3& location, const Color& color, double unit_intensity_distance)
                : m_location(location),
                  m_color(color),
                  m_unit_intensity_distance_square(unit_intensity_distance * unit_intensity_distance)
        {
        }
        void properties(const vec3& point, Color* color, vec3* vector_from_point_to_light) const override
        {
                *vector_from_point_to_light = m_location - point;
                *color = m_color *
                         (m_unit_intensity_distance_square / dot(*vector_from_point_to_light, *vector_from_point_to_light));
        }
};

class ConstantLight final : public LightSource
{
        vec3 m_location;
        Color m_color;

public:
        ConstantLight(const vec3& location, const Color& color) : m_location(location), m_color(color)
        {
        }
        void properties(const vec3& point, Color* color, vec3* vector_from_point_to_light) const override
        {
                *vector_from_point_to_light = m_location - point;
                *color = m_color;
        }
};
