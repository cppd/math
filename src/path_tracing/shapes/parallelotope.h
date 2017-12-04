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

#include "com/error.h"
#include "com/vec.h"
#include "geometry/core/array_elements.h"
#include "geometry/core/linear_algebra.h"
#include "path_tracing/ray.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace ParallelotopeImplementation
{
// Вспомогательная функция для следующей после неё функции
template <typename ObjectType, typename T, size_t... I>
constexpr ObjectType create_object_from_array_impl(const Vector<sizeof...(I), T>& org,
                                                   const std::array<Vector<sizeof...(I), T>, sizeof...(I)>& parameters,
                                                   std::integer_sequence<size_t, I...>)
{
        return ObjectType(org, parameters[I]...);
}
// Создать объект, передавая отдельные элементы массива как отдельные параметры конструктора этого объекта
template <typename ObjectType, size_t N, typename T>
constexpr ObjectType create_object_from_array(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& parameters)
{
        return create_object_from_array_impl<ObjectType>(org, parameters, std::make_integer_sequence<size_t, N>());
}
}

template <size_t N, typename T>
class Parallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Количество объектов после деления по каждому измерению
        static constexpr size_t DIVISIONS = 1 << N;

        struct Planes
        {
                Vector<N, T> n;
                T d1, d2;

        } m_planes[N];

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N> m_vectors;

        void reverse_planes(Planes* p);
        void create_planes();
        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);

public:
        Parallelotope() = default;

        template <typename... P>
        Parallelotope(const Vector<N, T>& org, const P&... vectors);

        bool inside(const Vector<N, T>& p) const;

        bool intersect(const Ray<N, T>& r, T epsilon, T intersection_threshold, T* t) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        template <typename ObjectType = Parallelotope<N, T>>
        std::array<ObjectType, DIVISIONS> binary_division() const;

        const Vector<N, T>& org() const;

        template <size_t Index>
        const Vector<N, T>& e() const;
};

// Параметр vectors — это все только Vector<N, T>
template <size_t N, typename T>
template <typename... P>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N);

        set_data(org, {{vectors...}});
}

template <size_t N, typename T>
void Parallelotope<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        m_org = org;
        m_vectors = vectors;
        create_planes();
}

// Умножение уравнений плоскостей на -1.
template <size_t N, typename T>
void Parallelotope<N, T>::reverse_planes(Planes* planes)
{
        planes->n = -planes->n;
        planes->d1 = -planes->d1;
        planes->d2 = -planes->d2;
}

template <size_t N, typename T>
void Parallelotope<N, T>::create_planes()
{
        // расстояние от точки до плоскости
        // dot(p - org, normal) = dot(p, normal) - dot(org, normal) = dot(p, normal) - d
        //
        // Вектор n наружу от объекта предназначен для плоскости с параметром d1.
        // Вектор -n наружу от объекта предназначен для плоскости с параметром d2.

        // Если векторы плоскостей получаются направленными внутрь параллелепипеда,
        // то надо умножить уравнения на - 1.

        for (unsigned i = 0; i < N; ++i)
        {
                m_planes[i].n = normalize(ortho_nn(del_elem(m_vectors, i)));
                m_planes[i].d1 = dot(m_org, m_planes[i].n);
                m_planes[i].d2 = -dot(m_org + m_vectors[i], m_planes[i].n);
                if (dot(m_planes[i].n, m_vectors[i]) > 0)
                {
                        reverse_planes(&m_planes[i]);
                }
        }
}

template <size_t N, typename T>
bool Parallelotope<N, T>::intersect(const Ray<N, T>& r, T epsilon, T intersection_threshold, T* t) const
{
        T f_max = std::numeric_limits<T>::lowest();
        T b_min = std::numeric_limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                T s = dot(r.get_dir(), m_planes[i].n);
                if (std::abs(s) < epsilon)
                {
                        T d = dot(r.get_org(), m_planes[i].n);
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

                T d = dot(r.get_org(), m_planes[i].n);
                T alpha1 = (m_planes[i].d1 - d) / s;
                // d и s имеют противоположный знак для другой плоскости
                T alpha2 = (m_planes[i].d2 + d) / -s;

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

        *t = (f_max > intersection_threshold) ? f_max : b_min;

        return *t > intersection_threshold;
}

template <size_t N, typename T>
Vector<N, T> Parallelotope<N, T>::normal(const Vector<N, T>& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        T min = std::numeric_limits<T>::max();
        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                T d = dot(p, m_planes[i].n);
                T l;

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

        ASSERT(min < std::numeric_limits<T>::max());

        return n;
}

template <size_t N, typename T>
bool Parallelotope<N, T>::inside(const Vector<N, T>& p) const
{
        // Надо использовать >, не >=.
        for (unsigned i = 0; i < N; ++i)
        {
                T d = dot(p, m_planes[i].n);

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

template <size_t N, typename T>
template <typename ObjectType>
std::array<ObjectType, Parallelotope<N, T>::DIVISIONS> Parallelotope<N, T>::binary_division() const
{
        std::array<Vector<N, T>, N> half_vectors;
        for (unsigned i = 0; i < N; ++i)
        {
                half_vectors[i] = m_vectors[i] / static_cast<T>(2);
        }

        std::array<ObjectType, DIVISIONS> res;

        // Если имеется 0 в разряде i номера объекта, то без смещения от начала объекта по измерению i.
        // Если имеется 1 в разряде i номера объекта, то со смещением от начала объекта по измерению i.
        static_assert(N <= 32);
        for (unsigned division = 0; division < DIVISIONS; ++division)
        {
                Vector<N, T> org = m_org;

                for (unsigned i = 0; i < N; ++i)
                {
                        if (division & (1u << i))
                        {
                                org += half_vectors[i];
                        }
                }

                res[division] = ParallelotopeImplementation::create_object_from_array<ObjectType>(org, half_vectors);
        }

        return res;
}

template <size_t N, typename T>
const Vector<N, T>& Parallelotope<N, T>::org() const
{
        return m_org;
}

template <size_t N, typename T>
template <size_t Index>
const Vector<N, T>& Parallelotope<N, T>::e() const
{
        static_assert(Index < N);
        return m_vectors[Index];
}
