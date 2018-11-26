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

#include "painter/lights/light_source.h"

template <size_t N, typename T>
class VisiblePointLight final : public LightSource<N, T>
{
        PointLight<N, T> m_light;

public:
        VisiblePointLight(const Vector<N, T>& location, const Color& color, T unit_intensity_distance)
                : m_light(location, color, unit_intensity_distance)
        {
        }

        void properties(const Vector<N, T>& point, Color* color, Vector<N, T>* vector_from_point_to_light) const override
        {
                m_light.properties(point, color, vector_from_point_to_light);
        }
};

template <size_t N, typename T>
class VisibleConstantLight final : public LightSource<N, T>
{
        ConstantLight<N, T> m_light;

public:
        VisibleConstantLight(const Vector<N, T>& location, const Color& color) : m_light(location, color)
        {
        }

        void properties(const Vector<N, T>& point, Color* color, Vector<N, T>* vector_from_point_to_light) const override
        {
                m_light.properties(point, color, vector_from_point_to_light);
        }
};
