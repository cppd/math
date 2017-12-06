/*
Copyright (C) 2017 Topological Manifold

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

#include "intersection.h"

#include "com/ray.h"
#include "com/vec.h"
#include "path_tracing/objects.h"

class Rectangle final : public IntersectionRectangle, public GeometricObject
{
        vec3 m_org, m_e0, m_e1;
        vec3 m_normal;
        vec3 m_u_beta, m_u_gamma;

public:
        Rectangle() = default;

        Rectangle(const vec3& org, const vec3& e0, const vec3& e1);
        void set_data(const vec3& org, const vec3& e0, const vec3& e1);

        bool intersect(const ray3& r, double* t) const override;

        vec3 normal(const vec3& point) const;

        const vec3& org() const override;
        const vec3& e0() const override;
        const vec3& e1() const override;
};
