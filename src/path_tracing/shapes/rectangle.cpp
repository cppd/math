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

#include "rectangle.h"

Rectangle::Rectangle(const vec3& org, const vec3& e0, const vec3& e1)
{
        set_data(org, e0, e1);
}

void Rectangle::set_data(const vec3& org, const vec3& e0, const vec3& e1)
{
        m_org = org;
        m_e0 = e0;
        m_e1 = e1;

        m_normal = normalize(cross(e0, e1));

        m_geometry.set_data(m_normal, m_org, {{m_e0, m_e1}});
}

bool Rectangle::intersect(const ray3& r, double* t) const
{
        return m_geometry.intersect(r, m_org, m_normal, t);
}
vec3 Rectangle::normal(const vec3&) const
{
        return m_normal;
}

const vec3& Rectangle::org() const
{
        return m_org;
}
const vec3& Rectangle::e0() const
{
        return m_e0;
}
const vec3& Rectangle::e1() const
{
        return m_e1;
}
