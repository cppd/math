/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "shape_intersection.h"

#include "com/error.h"
#include "com/print.h"
#include "com/ray.h"
#include "com/vec.h"
#include "painter/space/constraint.h"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace parallelotope_ortho_implementation
{
// Вспомогательная функция для следующей после неё функции
template <typename T, size_t ValueIndex, size_t... I>
constexpr Vector<sizeof...(I), T> index_vector_impl(T value, std::integer_sequence<size_t, I...>)
{
        return Vector<sizeof...(I), T>((I == ValueIndex ? value : 0)...);
}
// Вектор, в котором координата с индексом ValueIndex равна value, а остальные координаты равны 0
template <size_t N, typename T, size_t ValueIndex>
constexpr Vector<N, T> index_vector(T value)
{
        return index_vector_impl<T, ValueIndex>(value, std::make_integer_sequence<size_t, N>());
}

//

// Вспомогательная функция для следующей после неё функции
template <typename T, size_t... I>
constexpr Vector<sizeof...(I), T> index_vector_impl(unsigned index, T value, std::integer_sequence<size_t, I...>)
{
        return Vector<sizeof...(I), T>((I == index ? value : 0)...);
}
// Вектор, в котором координата с индексом index равна value, а остальные координаты равны 0
template <size_t N, typename T>
constexpr Vector<N, T> index_vector(unsigned index, T value)
{
        return index_vector_impl<T>(index, value, std::make_integer_sequence<size_t, N>());
}

//

// Вспомогательная функция для следующей после неё функции
template <typename T, size_t... I>
constexpr std::array<Vector<sizeof...(I), T>, sizeof...(I)> index_vectors_impl(T value, std::integer_sequence<size_t, I...>)
{
        return {index_vector<sizeof...(I), T, I>(value)...};
}
// Массив векторов, в котором вектор с индексом i имеет координату с индексом i, равную value.
// Пример: {( value, 0, 0), (0,  value, 0), (0, 0,  value)},
template <size_t N, typename T>
constexpr std::array<Vector<N, T>, N> index_vectors(T value)
{
        return index_vectors_impl<T>(value, std::make_integer_sequence<size_t, N>());
}

//

// Вспомогательная функция для следующей после неё функции
template <typename ObjectType, typename T, size_t... I>
constexpr ObjectType create_object_from_array_impl(const Vector<sizeof...(I), T>& org,
                                                   const std::array<T, sizeof...(I)>& parameters,
                                                   std::integer_sequence<size_t, I...>)
{
        return ObjectType(org, parameters[I]...);
}
// Создать объект, передавая отдельные элементы массива как отдельные параметры конструктора этого объекта
template <typename ObjectType, size_t N, typename T>
constexpr ObjectType create_object_from_array(const Vector<N, T>& org, const std::array<T, N>& parameters)
{
        return create_object_from_array_impl<ObjectType>(org, parameters, std::make_integer_sequence<size_t, N>());
}
}

template <size_t N, typename T>
class ParallelotopeOrtho final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Пример массива: {( 1, 0, 0), (0,  1, 0), (0, 0,  1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_POSITIVE =
                parallelotope_ortho_implementation::index_vectors<N, T>(1);
        // Пример массива: {(-1, 0, 0), (0, -1, 0), (0, 0, -1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_NEGATIVE =
                parallelotope_ortho_implementation::index_vectors<N, T>(-1);

        // Количество объектов после деления по каждому измерению
        static_assert(N <= 32);
        static constexpr size_t DIVISIONS = 1ull << N;

        struct Planes
        {
                T d1, d2;

        } m_planes[N];

        Vector<N, T> m_org;
        std::array<T, N> m_sizes;

        void create_planes();
        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        void set_data(const Vector<N, T>& org, const std::array<T, N>& sizes);

        bool intersect_impl(const Ray<N, T>& r, T* first, T* second) const;

public:
        static constexpr size_t DIMENSION = N;
        using DataType = T;

        ParallelotopeOrtho() = default;

        template <typename... P>
        ParallelotopeOrtho(const Vector<N, T>& org, const P&... vectors);

        ParallelotopeOrtho(const Vector<N, T>& org, const std::array<T, N>& vectors);

        void constraints(std::array<Constraint<N, T>, 2 * N>* c) const;

        bool inside(const Vector<N, T>& p) const;

        bool intersect(const Ray<N, T>& r, T* t) const;
        bool intersect_farthest(const Ray<N, T>& r, T* t) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<ParallelotopeOrtho<N, T>, DIVISIONS> binary_division() const;

        const Vector<N, T>& org() const;

        Vector<N, T> e(unsigned n) const;
};

// Параметр vectors — это или все только T, или все только Vector<N, T>
template <size_t N, typename T>
template <typename... P>
ParallelotopeOrtho<N, T>::ParallelotopeOrtho(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<T, P> && ...) || (std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N);

        if constexpr ((std::is_same_v<T, P> && ...))
        {
                set_data(org, std::array<T, N>{vectors...});
        }
        if constexpr ((std::is_same_v<Vector<N, T>, P> && ...))
        {
                set_data(org, std::array<Vector<N, T>, N>{vectors...});
        }
}

template <size_t N, typename T>
ParallelotopeOrtho<N, T>::ParallelotopeOrtho(const Vector<N, T>& org, const std::array<T, N>& vectors)
{
        set_data(org, vectors);
}

template <size_t N, typename T>
void ParallelotopeOrtho<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        std::array<T, N> data;

        for (unsigned vector_number = 0; vector_number < N; ++vector_number)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (i != vector_number && vectors[vector_number][i] != 0)
                        {
                                error("Error orthogonal parallelotope vectors");
                        }
                }

                data[vector_number] = vectors[vector_number][vector_number];
        }

        set_data(org, data);
}

template <size_t N, typename T>
void ParallelotopeOrtho<N, T>::set_data(const Vector<N, T>& org, const std::array<T, N>& sizes)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!(sizes[i] > 0))
                {
                        error("Error orthogonal parallelotope sizes");
                }
        }

        m_org = org;
        m_sizes = sizes;

        // d1 для нормалей с положительной координатой NORMALS_POSITIVE
        // d2 для нормалей с отрицательной координатой NORMALS_NEGATIVE
        // Уравнения плоскостей, перпендикулярных измерению 0
        // x - (org[0] + x) = 0
        // -x - (-org[0]) = 0

        for (unsigned i = 0; i < N; ++i)
        {
                m_planes[i].d1 = m_org[i] + m_sizes[i];
                m_planes[i].d2 = -m_org[i];
        }
}

// Неравенства в виде b + a * x >= 0, задающие множество точек параллелотопа.
template <size_t N, typename T>
void ParallelotopeOrtho<N, T>::constraints(std::array<Constraint<N, T>, 2 * N>* c) const
{
        // Плоскости n * x - d имеют перпендикуляр с направлением наружу.
        // Направление внутрь -n * x + d или d + -(n * x), тогда условие
        // для точек параллелотопа d + -(n * x) >= 0.

        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                (*c)[c_i].a = NORMALS_NEGATIVE[i];
                (*c)[c_i].b = m_planes[i].d1;

                (*c)[c_i + 1].a = NORMALS_POSITIVE[i];
                (*c)[c_i + 1].b = m_planes[i].d2;
        }
}

template <size_t N, typename T>
bool ParallelotopeOrtho<N, T>::intersect_impl(const Ray<N, T>& r, T* first, T* second) const
{
        T f_max = std::numeric_limits<T>::lowest();
        T b_min = std::numeric_limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                T s = r.dir()[i]; // dot(r.dir(), m_planes[i].n);
                if (s == 0)
                {
                        T d = r.org()[i]; // dot(r.org(), m_planes[i].n);
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

                T d = r.org()[i]; // dot(r.org(), m_planes[i].n);
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
bool ParallelotopeOrtho<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        T first, second;
        if (intersect_impl(r, &first, &second))
        {
                *t = (first > 0) ? first : second;
                return true;
        }
        else
        {
                return false;
        }
}

template <size_t N, typename T>
bool ParallelotopeOrtho<N, T>::intersect_farthest(const Ray<N, T>& r, T* t) const
{
        T first, second;
        if (intersect_impl(r, &first, &second))
        {
                *t = second;
                return true;
        }
        else
        {
                return false;
        }
}

template <size_t N, typename T>
Vector<N, T> ParallelotopeOrtho<N, T>::normal(const Vector<N, T>& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        T min = std::numeric_limits<T>::max();

        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                if (T l = std::abs(p[i] - m_planes[i].d1); l < min)
                {
                        min = l;
                        n = NORMALS_POSITIVE[i];
                }

                if (T l = std::abs(p[i] + m_planes[i].d2); l < min)
                {
                        min = l;
                        n = NORMALS_NEGATIVE[i];
                }
        }

        ASSERT(min < std::numeric_limits<T>::max());

        return n;
}

template <size_t N, typename T>
bool ParallelotopeOrtho<N, T>::inside(const Vector<N, T>& p) const
{
        for (unsigned i = 0; i < N; ++i)
        {
                // Надо использовать <=, не <.
                if (!(p[i] <= m_planes[i].d1) || !(-p[i] <= m_planes[i].d2))
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, typename T>
std::array<ParallelotopeOrtho<N, T>, ParallelotopeOrtho<N, T>::DIVISIONS> ParallelotopeOrtho<N, T>::binary_division() const
{
        std::array<T, N> half_sizes;
        for (unsigned i = 0; i < N; ++i)
        {
                half_sizes[i] = m_sizes[i] / 2;
        }

        std::array<T, N> org_plus_half;
        for (unsigned i = 0; i < N; ++i)
        {
                org_plus_half[i] = m_org[i] + half_sizes[i];
        }

        std::array<ParallelotopeOrtho, DIVISIONS> res;

        // Если имеется 0 в разряде i номера объекта, то без смещения от начала объекта по измерению i.
        // Если имеется 1 в разряде i номера объекта, то со смещением от начала объекта по измерению i.
        static_assert(N <= 32);
        for (size_t division = 0; division < DIVISIONS; ++division)
        {
                Vector<N, T> org;
                for (unsigned i = 0; i < N; ++i)
                {
                        org[i] = (division & (1u << i)) ? org_plus_half[i] : m_org[i];
                }

                res[division] = parallelotope_ortho_implementation::create_object_from_array<ParallelotopeOrtho>(org, half_sizes);
        }

        return res;
}

template <size_t N, typename T>
const Vector<N, T>& ParallelotopeOrtho<N, T>::org() const
{
        return m_org;
}

template <size_t N, typename T>
Vector<N, T> ParallelotopeOrtho<N, T>::e(unsigned n) const
{
        ASSERT(n < N);
        return parallelotope_ortho_implementation::index_vector<N, T>(n, m_sizes[n]);
}

template <size_t N, typename T>
std::string to_string(const ParallelotopeOrtho<N, T>& p)
{
        std::string s;
        s += "org = " + to_string(p.org()) + "\n";
        for (unsigned i = 0; i < N; ++i)
        {
                s += "edge[" + to_string(i) + "] = " + to_string(p.e(i)) + ((i < N - 1) ? "\n" : "");
        }
        return s;
}
