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

#include "objects.h"

#include "path_tracing/lights/light_source.h"

class VisiblePointLight final : public LightSource
{
        PointLight<3, double> m_light;

public:
        VisiblePointLight(const vec3& location, const Color& color, double unit_intensity_distance)
                : m_light(location, color, unit_intensity_distance)
        {
        }

        void properties(const vec3& point, Color* color, vec3* vector_from_point_to_light) const override
        {
                m_light.properties(point, color, vector_from_point_to_light);
        }
};

class VisibleConstantLight final : public LightSource
{
        ConstantLight<3, double> m_light;

public:
        VisibleConstantLight(const vec3& location, const Color& color) : m_light(location, color)
        {
        }

        void properties(const vec3& point, Color* color, vec3* vector_from_point_to_light) const override
        {
                m_light.properties(point, color, vector_from_point_to_light);
        }
};
