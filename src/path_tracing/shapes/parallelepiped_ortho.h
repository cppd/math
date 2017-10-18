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

class ParallelepipedOrtho final : public GeometricParallelepiped
{
        static constexpr vec3 NORMALS[3] = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};
        static constexpr vec3 NORMALS_R[3] = {vec3(-1, 0, 0), vec3(0, -1, 0), vec3(0, 0, -1)};

        struct Planes
        {
                double d1, d2;

        } m_planes[3];

        vec3 m_org;
        double m_x, m_y, m_z;

        void create_planes();

public:
        ParallelepipedOrtho() = default;

        ParallelepipedOrtho(const vec3& org, double x, double y, double z);
        ParallelepipedOrtho(const vec3& org, const vec3& x, const vec3& y, const vec3& z);

        void set_data(const vec3& org, double x, double y, double z);

        bool inside(const vec3& p) const override;
        bool intersect(const ray3& r, double* t) const override;

        vec3 normal(const vec3& p) const;

        void binary_division(std::array<ParallelepipedOrtho, 8>* p) const;
        std::array<ParallelepipedOrtho, 8> binary_division() const;

        const vec3& org() const override;
        vec3 e0() const override;
        vec3 e1() const override;
        vec3 e2() const override;
};
