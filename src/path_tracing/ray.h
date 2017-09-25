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

#include "vec3.h"

class Ray
{
        vec3 m_org;
        vec3 m_dir;

public:
        Ray() = default;

        Ray(const vec3& org, const vec3& dir) : m_org(org), m_dir(normalize(dir))
        {
        }

        void set_org(const vec3& org)
        {
                m_org = org;
        }

        void set_dir(const vec3& dir)
        {
                m_dir = normalize(dir);
        }

        const vec3& get_org() const
        {
                return m_org;
        }

        const vec3& get_dir() const
        {
                return m_dir;
        }

        vec3 point(double t) const
        {
                return m_org + m_dir * t;
        }
};
