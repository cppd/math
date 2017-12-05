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

#include "base.h"
#include "parallelotope.h"

#include "path_tracing/constants.h"

class Parallelepiped final : public GeometricParallelepiped
{
        Parallelotope<3, double> m_parallelepiped;

public:
        Parallelepiped() = default;
        Parallelepiped(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2) : m_parallelepiped(org, e0, e1, e2)
        {
        }
        bool inside(const vec3& p) const override
        {
                return m_parallelepiped.inside(p);
        }
        bool intersect(const ray3& r, double* t) const override
        {
                return m_parallelepiped.intersect(r, EPSILON, INTERSECTION_THRESHOLD, t);
        }
        vec3 normal(const vec3& p) const
        {
                return m_parallelepiped.normal(p);
        }
        std::array<Parallelepiped, 8> binary_division() const
        {
                return m_parallelepiped.binary_division<Parallelepiped>();
        }
        const vec3& org() const override
        {
                return m_parallelepiped.org();
        }
        vec3 e0() const override
        {
                return m_parallelepiped.e<0>();
        }
        vec3 e1() const override
        {
                return m_parallelepiped.e<1>();
        }
        vec3 e2() const override
        {
                return m_parallelepiped.e<2>();
        }
};
