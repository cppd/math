/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "constraint.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/orthogonal.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <utility>

namespace painter
{
template <size_t N, typename T>
class Parallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Количество объектов после деления по каждому измерению
        static_assert(N <= 32);
        static constexpr size_t DIVISIONS = 1ull << N;

        struct Planes
        {
                Vector<N, T> n;
                T d1, d2;
        };
        std::array<Planes, N> m_planes;

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N> m_vectors;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);

        bool intersect_impl(const Ray<N, T>& r, T* first, T* second) const;

public:
        static constexpr size_t DIMENSION = N;
        using DataType = T;

        Parallelotope() = default;

        template <typename... P>
        explicit Parallelotope(const Vector<N, T>& org, const P&... vectors);

        Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);

        void constraints(std::array<Constraint<N, T>, 2 * N>* c) const;

        bool inside(const Vector<N, T>& p) const;

        bool intersect(const Ray<N, T>& r, T* t) const;
        bool intersect_farthest(const Ray<N, T>& r, T* t) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<Parallelotope<N, T>, DIVISIONS> binary_division() const;

        const Vector<N, T>& org() const;

        const Vector<N, T>& e(unsigned n) const;
};

// Параметр vectors — это все только Vector<N, T>
template <size_t N, typename T>
template <typename... P>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N);

        set_data(org, {vectors...});
}

template <size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        set_data(org, vectors);
}

template <size_t N, typename T>
void Parallelotope<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        m_org = org;
        m_vectors = vectors;

        // Расстояние от точки до плоскости
        // dot(p - org, normal) = dot(p, normal) - dot(org, normal)
        // d = dot(org, normal)
        // Вектор n наружу от объекта предназначен для плоскости с параметром d2.
        for (unsigned i = 0; i < N; ++i)
        {
                m_planes[i].n = ortho_nn(del_elem(m_vectors, i)).normalized();
                if (dot(m_planes[i].n, m_vectors[i]) < 0)
                {
                        m_planes[i].n = -m_planes[i].n;
                }
                m_planes[i].d1 = dot(m_org, m_planes[i].n);
                m_planes[i].d2 = dot(m_org + m_vectors[i], m_planes[i].n);
        }
}

// Неравенства в виде b + a * x >= 0, задающие множество точек параллелотопа.
template <size_t N, typename T>
void Parallelotope<N, T>::constraints(std::array<Constraint<N, T>, 2 * N>* c) const
{
        // Плоскости n * x - d имеют перпендикуляр с направлением наружу.
        // Направление внутрь -n * x + d или d + -(n * x), тогда условие
        // для точек параллелотопа d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                (*c)[c_i].a = m_planes[i].n;
                (*c)[c_i].b = -m_planes[i].d1;

                (*c)[c_i + 1].a = -m_planes[i].n;
                (*c)[c_i + 1].b = m_planes[i].d2;
        }
}

template <size_t N, typename T>
bool Parallelotope<N, T>::intersect_impl(const Ray<N, T>& r, T* first, T* second) const
{
        T f_max = limits<T>::lowest();
        T b_min = limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                T s = dot(r.dir(), m_planes[i].n);
                if (s == 0)
                {
                        T d = dot(r.org(), m_planes[i].n);
                        if (d < m_planes[i].d1 || d > m_planes[i].d2)
                        {
                                // параллельно плоскостям и снаружи
                                return false;
                        }
                        // внутри плоскостей
                        continue;
                }

                T d = dot(r.org(), m_planes[i].n);
                T alpha1 = (m_planes[i].d1 - d) / s;
                T alpha2 = (m_planes[i].d2 - d) / s;

                if (s > 0)
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

                if (b_min <= 0 || b_min < f_max)
                {
                        return false;
                }
        }

        *first = f_max;
        *second = b_min;

        return true;
}

template <size_t N, typename T>
bool Parallelotope<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                *t = (first > 0) ? first : second;
                return true;
        }
        return false;
}

template <size_t N, typename T>
bool Parallelotope<N, T>::intersect_farthest(const Ray<N, T>& r, T* t) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                *t = second;
                return true;
        }
        return false;
}

template <size_t N, typename T>
Vector<N, T> Parallelotope<N, T>::normal(const Vector<N, T>& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        T min = limits<T>::max();

        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                T d = dot(p, m_planes[i].n);
                T l;

                l = std::abs(d - m_planes[i].d1);
                if (l < min)
                {
                        min = l;
                        n = -m_planes[i].n;
                }

                l = std::abs(d - m_planes[i].d2);
                if (l < min)
                {
                        min = l;
                        n = m_planes[i].n;
                }
        }

        ASSERT(min < limits<T>::max());

        return n;
}

template <size_t N, typename T>
bool Parallelotope<N, T>::inside(const Vector<N, T>& p) const
{
        for (unsigned i = 0; i < N; ++i)
        {
                T d = dot(p, m_planes[i].n);

                if (d < m_planes[i].d1)
                {
                        return false;
                }

                if (d > m_planes[i].d2)
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, typename T>
std::array<Parallelotope<N, T>, Parallelotope<N, T>::DIVISIONS> Parallelotope<N, T>::binary_division() const
{
        std::array<Parallelotope, DIVISIONS> res;

        std::array<Vector<N, T>, N> half_vectors;
        Vector<N, T> middle_org = m_org;
        Vector<N, T> middle_plane_d;
        for (unsigned i = 0; i < N; ++i)
        {
                half_vectors[i] = m_vectors[i] / static_cast<T>(2);
                middle_org += half_vectors[i];
                middle_plane_d[i] = (m_planes[i].d2 + m_planes[i].d1) / static_cast<T>(2);
        }

        for (size_t division = 0; division < DIVISIONS; ++division)
        {
                res[division].m_vectors = half_vectors;
                for (unsigned i = 0; i < N; ++i)
                {
                        res[division].m_planes[i].n = m_planes[i].n;
                }
        }

        for (size_t division = 0; division < DIVISIONS; ++division)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if ((division & (1u << i)) != 0)
                        {
                                res[division].m_org[i] = middle_org[i];
                                res[division].m_planes[i].d1 = middle_plane_d[i];
                                res[division].m_planes[i].d2 = m_planes[i].d2;
                        }
                        else
                        {
                                res[division].m_org[i] = m_org[i];
                                res[division].m_planes[i].d1 = m_planes[i].d1;
                                res[division].m_planes[i].d2 = middle_plane_d[i];
                        }
                }
        }

        return res;
}

template <size_t N, typename T>
const Vector<N, T>& Parallelotope<N, T>::org() const
{
        return m_org;
}

template <size_t N, typename T>
const Vector<N, T>& Parallelotope<N, T>::e(unsigned n) const
{
        ASSERT(n < N);
        return m_vectors[n];
}
}

template <size_t N, typename T>
std::string to_string(const painter::Parallelotope<N, T>& p)
{
        std::string s;
        s += "org = " + to_string(p.org()) + "\n";
        for (unsigned i = 0; i < N; ++i)
        {
                s += "edge[" + to_string(i) + "] = " + to_string(p.e(i)) + ((i < N - 1) ? "\n" : "");
        }
        return s;
}
