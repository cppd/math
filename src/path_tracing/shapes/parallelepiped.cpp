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

// Формулы имеются в книге
// Samuel R. Buss.
// 3D Computer Graphics. A Mathematical Introduction with OpenGL.
// Cambridge University Press, 2003.

#include "parallelepiped.h"

#include "com/error.h"
#include "path_tracing/constants.h"

#include <algorithm>
#include <limits>

Parallelepiped::Parallelepiped(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2)
        : m_org(org), m_e0(e0), m_e1(e1), m_e2(e2)
{
        create_planes();
}

void Parallelepiped::set_data(const vec3& org, const vec3& e0, const vec3& e1, const vec3& e2)
{
        m_org = org;
        m_e0 = e0;
        m_e1 = e1;
        m_e2 = e2;

        create_planes();
}

// Умножение уравнений плоскостей на -1.
void Parallelepiped::reverse_planes(Planes* planes)
{
        planes->n = -planes->n;
        planes->d1 = -planes->d1;
        planes->d2 = -planes->d2;
}

void Parallelepiped::create_planes()
{
        // расстояние от точки до плоскости
        // dot(p - org, normal) = dot(p, normal) - dot(org, normal) = dot(p, normal) - d
        //
        // Вектор n наружу от объекта предназначен для плоскости с параметром d1.
        // Вектор -n наружу от объекта предназначен для плоскости с параметром d2.

        // Если векторы плоскостей получаются направленными внутрь параллелепипеда,
        // то надо умножить уравнения на - 1.

        m_planes[0].n = normalize(cross(m_e0, m_e1));
        m_planes[0].d1 = dot(m_org, m_planes[0].n);
        m_planes[0].d2 = -dot(m_org + m_e2, m_planes[0].n);
        if (dot(m_planes[0].n, m_e2) > 0)
        {
                reverse_planes(&m_planes[0]);
        }

        m_planes[1].n = normalize(cross(m_e1, m_e2));
        m_planes[1].d1 = dot(m_org, m_planes[1].n);
        m_planes[1].d2 = -dot(m_org + m_e0, m_planes[1].n);
        if (dot(m_planes[1].n, m_e0) > 0)
        {
                reverse_planes(&m_planes[1]);
        }

        m_planes[2].n = normalize(cross(m_e2, m_e0));
        m_planes[2].d1 = dot(m_org, m_planes[2].n);
        m_planes[2].d2 = -dot(m_org + m_e1, m_planes[2].n);
        if (dot(m_planes[2].n, m_e1) > 0)
        {
                reverse_planes(&m_planes[2]);
        }
}

bool Parallelepiped::intersect(const ray3& r, double* t) const
{
        double f_max = std::numeric_limits<double>::lowest();
        double b_min = std::numeric_limits<double>::max();

        for (int i = 0; i < 3; ++i)
        {
                double s = dot(r.get_dir(), m_planes[i].n);
                if (std::abs(s) < EPSILON)
                {
                        double d = dot(r.get_org(), m_planes[i].n);
                        if (d - m_planes[i].d1 > 0 || -d - m_planes[i].d2 > 0)
                        {
                                // параллельно плоскостям и снаружи
                                return false;
                        }
                        else
                        {
                                // внутри плоскостей
                                continue;
                        }
                }

                double d = dot(r.get_org(), m_planes[i].n);
                double alpha1 = (m_planes[i].d1 - d) / s;
                // d и s имеют противоположный знак для другой плоскости
                double alpha2 = (m_planes[i].d2 + d) / -s;

                if (s < 0)
                {
                        // пересечение снаружи для первой плоскости
                        // пересечение внутри для второй плоскости
                        f_max = std::max(alpha1, f_max);
                        b_min = std::min(alpha2, b_min);
                }
                else
                {
                        // пересечение внутри для первой плоскости
                        // пересечение снаружи для второй плоскости
                        b_min = std::min(alpha1, b_min);
                        f_max = std::max(alpha2, f_max);
                }

                if (b_min < 0 || b_min < f_max)
                {
                        return false;
                }
        }

        *t = (f_max > INTERSECTION_THRESHOLD) ? f_max : b_min;

        return *t > INTERSECTION_THRESHOLD;
}

vec3 Parallelepiped::normal(const vec3& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        double min = std::numeric_limits<double>::max();
        vec3 n;
        for (int i = 0; i < 3; ++i)
        {
                double d = dot(p, m_planes[i].n);
                double l;

                l = std::abs(d - m_planes[i].d1);
                if (l < min)
                {
                        min = l;
                        n = m_planes[i].n;
                }

                l = std::abs(-d - m_planes[i].d2);
                if (l < min)
                {
                        min = l;
                        n = -m_planes[i].n;
                }
        }

        ASSERT(min < std::numeric_limits<double>::max());

        return n;
}

bool Parallelepiped::inside(const vec3& p) const
{
        // Надо использовать >, не >=.
        for (int i = 0; i < 3; ++i)
        {
                double d = dot(p, m_planes[i].n);

                if (d - m_planes[i].d1 > 0)
                {
                        // на внешней стороне
                        return false;
                }

                if (-d - m_planes[i].d2 > 0)
                {
                        // на внешней стороне
                        return false;
                }
        }
        return true;
}

const vec3& Parallelepiped::org() const
{
        return m_org;
}

vec3 Parallelepiped::e0() const
{
        return m_e0;
}

vec3 Parallelepiped::e1() const
{
        return m_e1;
}

vec3 Parallelepiped::e2() const
{
        return m_e2;
}

void Parallelepiped::binary_division(std::array<Parallelepiped, 8>* p) const
{
        vec3 half0 = m_e0 / 2.0;
        vec3 half1 = m_e1 / 2.0;
        vec3 half2 = m_e2 / 2.0;

        std::array<vec3, 8> orgs;

        orgs[0] = m_org;
        orgs[1] = orgs[0] + half0;
        orgs[2] = orgs[0] + half1;
        orgs[3] = orgs[2] + half0;
        orgs[4] = m_org + half2;
        orgs[5] = orgs[4] + half0;
        orgs[6] = orgs[4] + half1;
        orgs[7] = orgs[6] + half0;

        for (int i = 0; i < 8; ++i)
        {
                (*p)[i] = Parallelepiped(orgs[i], half0, half1, half2);
        }
}

std::array<Parallelepiped, 8> Parallelepiped::binary_division() const
{
        std::array<Parallelepiped, 8> res;
        binary_division(&res);
        return res;
}
