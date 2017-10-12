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

#include "com/vec.h"
#include "path_tracing/objects.h"
#include "path_tracing/ray.h"

#include <array>

class Parallelepiped final : public GeometricParallelepiped
{
        struct Planes
        {
                vec3 n;
                double d1, d2;

        } m_planes[3];

        vec3 m_org, m_e0, m_e1, m_e2;

        void reverse_planes(Planes* p);

        void create_planes();

public:
        Parallelepiped() = default;

        Parallelepiped(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2);

        void set_data(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2);

        bool inside(const vec3& p) const override;
        bool intersect(const ray3& r, double* t) const override;

        vec3 normal(const vec3& p) const;

        void binary_division(std::array<Parallelepiped, 8>* p) const;

        const vec3& org() const override;
        vec3 e0() const override;
        vec3 e1() const override;
        vec3 e2() const override;
};
