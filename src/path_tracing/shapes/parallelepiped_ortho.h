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

/*
 Формулы имеются в книге

 Samuel R. Buss.
 3D Computer Graphics. A Mathematical Introduction with OpenGL.
 Cambridge University Press, 2003.
*/

#pragma once

#include "base.h"
#include "parallelotope_ortho.h"
#include "path_tracing/constants.h"

class ParallelepipedOrtho final : public GeometricParallelepiped
{
        ParallelotopeOrtho<3, double> m_parallelepiped_ortho;

public:
        ParallelepipedOrtho() = default;
        ParallelepipedOrtho(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2)
                : m_parallelepiped_ortho(org, std::array<vec3, 3>{{e0, e1, e2}})
        {
        }
        ParallelepipedOrtho(const vec3& org, double e0, double e1, double e2)
                : m_parallelepiped_ortho(org, std::array<double, 3>{{e0, e1, e2}})
        {
        }
        ParallelepipedOrtho(const vec3& org, const std::array<double, 3>& sizes) : m_parallelepiped_ortho(org, sizes)
        {
        }
        ParallelepipedOrtho(const vec3& org, const std::array<vec3, 3>& sizes) : m_parallelepiped_ortho(org, sizes)
        {
        }
        bool inside(const vec3& p) const override
        {
                return m_parallelepiped_ortho.inside(p);
        }
        bool intersect(const ray3& r, double* t) const override
        {
                return m_parallelepiped_ortho.intersect(r, INTERSECTION_THRESHOLD, t);
        }
        vec3 normal(const vec3& p) const
        {
                return m_parallelepiped_ortho.normal(p);
        }
        std::array<ParallelepipedOrtho, 8> binary_division() const
        {
                return m_parallelepiped_ortho.binary_division<ParallelepipedOrtho>();
        }
        const vec3& org() const override
        {
                return m_parallelepiped_ortho.org();
        }
        vec3 e0() const override
        {
                return m_parallelepiped_ortho.e<0>();
        }
        vec3 e1() const override
        {
                return m_parallelepiped_ortho.e<1>();
        }
        vec3 e2() const override
        {
                return m_parallelepiped_ortho.e<2>();
        }
};
