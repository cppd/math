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

#include "parallelepiped_ortho.h"

#include "com/error.h"
#include "path_tracing/constants.h"

#include <algorithm>
#include <limits>

ParallelepipedOrtho::ParallelepipedOrtho(const vec3& org, double x, double y, double z)
{
        set_data(org, x, y, z);
}

ParallelepipedOrtho::ParallelepipedOrtho(const vec3& org, const vec3& x, const vec3& y, const vec3& z)
{
        if (!(x[1] == 0 && x[2] == 0 && y[0] == 0 && y[2] == 0 && z[0] == 0 && z[1] == 0))
        {
                error("Error orthogonal parallelepiped vectors");
        }

        set_data(org, x[0], y[1], z[2]);
}

void ParallelepipedOrtho::set_data(const vec3& org, double x, double y, double z)
{
        if (!(x > 0 && y > 0 && z > 0))
        {
                error("Error orthogonal parallelepiped sizes");
        }

        m_org = org;
        m_x = x;
        m_y = y;
        m_z = z;

        // d1 для нормали с положительной координатой (1, 0, 0) (0, 1, 0) (0, 0, 1)
        // d2 для нормали с отрицательной координатой (-1, 0, 0) (0, -1, 0) (0, 0, -1)

        // Уравнения плоскостей, параллельных Y и Z
        // x - (org[0] + x) = 0
        // -x - (-org[0]) = 0
        m_planes[0].d1 = m_org[0] + x;
        m_planes[0].d2 = -m_org[0];

        // Уравнения плоскостей, параллельных X и Z
        // y - (org[1] + y) = 0
        // -y - (-org[1]) = 0
        m_planes[1].d1 = m_org[1] + y;
        m_planes[1].d2 = -m_org[1];

        // Уравнения плоскостей, параллельных X и Y
        // z - (org[2] + z) = 0
        // -z - (-org[2]) = 0
        m_planes[2].d1 = m_org[2] + z;
        m_planes[2].d2 = -m_org[2];
}

bool ParallelepipedOrtho::intersect(const ray3& r, double* t) const
{
        double f_max = std::numeric_limits<double>::lowest();
        double b_min = std::numeric_limits<double>::max();

        for (int i = 0; i < 3; ++i)
        {
                double s = r.get_dir()[i]; // dot(r.get_dir(), m_planes[i].n);
                if (s == 0)
                {
                        double d = r.get_org()[i]; // dot(r.get_org(), m_planes[i].n);
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

                double d = r.get_org()[i]; // dot(r.get_org(), m_planes[i].n);
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

vec3 ParallelepipedOrtho::normal(const vec3& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        double min = std::numeric_limits<double>::max();

        vec3 n;
        for (int i = 0; i < 3; ++i)
        {
                if (double l = std::abs(p[i] - m_planes[i].d1); l < min)
                {
                        min = l;
                        n = NORMALS[i];
                }

                if (double l = std::abs(p[i] + m_planes[i].d2); l < min)
                {
                        min = l;
                        n = NORMALS_R[i];
                }
        }

        ASSERT(min < std::numeric_limits<double>::max());

        return n;
}

bool ParallelepipedOrtho::inside(const vec3& p) const
{
        // Надо использовать <=, не <.
        return p[0] <= m_planes[0].d1 && -p[0] <= m_planes[0].d2 && p[1] <= m_planes[1].d1 && -p[1] <= m_planes[1].d2 &&
               p[2] <= m_planes[2].d1 && -p[2] <= m_planes[2].d2;
}

const vec3& ParallelepipedOrtho::org() const
{
        return m_org;
}

vec3 ParallelepipedOrtho::e0() const
{
        return vec3(m_x, 0, 0);
}

vec3 ParallelepipedOrtho::e1() const
{
        return vec3(0, m_y, 0);
}

vec3 ParallelepipedOrtho::e2() const
{
        return vec3(0, 0, m_z);
}

void ParallelepipedOrtho::binary_division(std::array<ParallelepipedOrtho, 8>* p) const
{
        double half0 = m_x / 2;
        double half1 = m_y / 2;
        double half2 = m_z / 2;

        double x0 = m_org[0];
        double x1 = m_org[0] + half0;
        double y0 = m_org[1];
        double y1 = m_org[1] + half1;
        double z0 = m_org[2];
        double z1 = m_org[2] + half2;

        (*p)[0] = ParallelepipedOrtho(vec3(x0, y0, z0), half0, half1, half2);
        (*p)[1] = ParallelepipedOrtho(vec3(x1, y0, z0), half0, half1, half2);
        (*p)[2] = ParallelepipedOrtho(vec3(x0, y1, z0), half0, half1, half2);
        (*p)[3] = ParallelepipedOrtho(vec3(x1, y1, z0), half0, half1, half2);
        (*p)[4] = ParallelepipedOrtho(vec3(x0, y0, z1), half0, half1, half2);
        (*p)[5] = ParallelepipedOrtho(vec3(x1, y0, z1), half0, half1, half2);
        (*p)[6] = ParallelepipedOrtho(vec3(x0, y1, z1), half0, half1, half2);
        (*p)[7] = ParallelepipedOrtho(vec3(x1, y1, z1), half0, half1, half2);
}

std::array<ParallelepipedOrtho, 8> ParallelepipedOrtho::binary_division() const
{
        std::array<ParallelepipedOrtho, 8> res;
        binary_division(&res);
        return res;
}
