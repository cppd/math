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

#include "vec.h"

template <size_t N, typename T>
class Ray
{
        Vector<N, T> m_org;
        Vector<N, T> m_dir;

public:
        Ray() = default;

        Ray(const Vector<N, T>& org, const Vector<N, T>& dir) : m_org(org), m_dir(normalize(dir))
        {
        }

        void set_org(const Vector<N, T>& org)
        {
                m_org = org;
        }

        void set_dir(const Vector<N, T>& dir)
        {
                m_dir = normalize(dir);
        }

        const Vector<N, T>& get_org() const
        {
                return m_org;
        }

        const Vector<N, T>& get_dir() const
        {
                return m_dir;
        }

        Vector<N, T> point(T t) const
        {
                return m_org + m_dir * t;
        }
};
