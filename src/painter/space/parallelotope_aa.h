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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <utility>

namespace ns::painter
{
namespace parallelotope_aa_implementation
{
// Вспомогательная функция для следующей после неё функции
template <typename T, std::size_t ValueIndex, std::size_t... I>
constexpr Vector<sizeof...(I), T> index_vector_impl(T value, std::integer_sequence<std::size_t, I...>)
{
        return Vector<sizeof...(I), T>((I == ValueIndex ? value : 0)...);
}
// Вектор, в котором координата с индексом ValueIndex равна value, а остальные координаты равны 0
template <std::size_t N, typename T, std::size_t ValueIndex>
constexpr Vector<N, T> index_vector(T value)
{
        return index_vector_impl<T, ValueIndex>(value, std::make_integer_sequence<std::size_t, N>());
}

//

// Вспомогательная функция для следующей после неё функции
template <typename T, std::size_t... I>
constexpr Vector<sizeof...(I), T> index_vector_impl(unsigned index, T value, std::integer_sequence<std::size_t, I...>)
{
        return Vector<sizeof...(I), T>((I == index ? value : 0)...);
}
// Вектор, в котором координата с индексом index равна value, а остальные координаты равны 0
template <std::size_t N, typename T>
constexpr Vector<N, T> index_vector(unsigned index, T value)
{
        return index_vector_impl<T>(index, value, std::make_integer_sequence<std::size_t, N>());
}

//

// Вспомогательная функция для следующей после неё функции
template <typename T, std::size_t... I>
constexpr std::array<Vector<sizeof...(I), T>, sizeof...(I)> index_vectors_impl(
        T value,
        std::integer_sequence<std::size_t, I...>)
{
        return {index_vector<sizeof...(I), T, I>(value)...};
}
// Массив векторов, в котором вектор с индексом i имеет координату с индексом i, равную value.
// Пример: {( value, 0, 0), (0,  value, 0), (0, 0,  value)},
template <std::size_t N, typename T>
constexpr std::array<Vector<N, T>, N> index_vectors(T value)
{
        return index_vectors_impl<T>(value, std::make_integer_sequence<std::size_t, N>());
}
}

template <std::size_t N, typename T>
class ParallelotopeAA final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Пример массива: {(1, 0, 0), (0, 1, 0), (0, 0, 1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_POSITIVE =
                parallelotope_aa_implementation::index_vectors<N, T>(1);
        // Пример массива: {(-1, 0, 0), (0, -1, 0), (0, 0, -1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_NEGATIVE =
                parallelotope_aa_implementation::index_vectors<N, T>(-1);

        static_assert(N <= 27);

        // Количество объектов после деления по каждому измерению
        static constexpr int DIVISIONS = 1 << N;

        static constexpr int VERTEX_COUNT = 1 << N;

        // Количество вершин 2 ^ N умножить на количество измерений N у каждой вершины
        // и для уникальности разделить на 2 = ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int VERTEX_RIDGE_COUNT = (1 << (N - 1)) * N;

        struct Planes
        {
                T d1, d2;
        };
        std::array<Planes, N> m_planes;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        void set_data(const Vector<N, T>& org, const std::array<T, N>& sizes);

        bool intersect_impl(const Ray<N, T>& r, T* first, T* second) const;

        T size(unsigned i) const;

        template <int INDEX, typename F>
        void binary_division_impl(std::array<Planes, N>* p, const Vector<N, T>& middle_d, const F& f) const;

        template <int INDEX, typename F>
        void vertices_impl(Vector<N, T>* p, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N;

        using DataType = T;

        ParallelotopeAA() = default;

        template <typename... P>
        explicit ParallelotopeAA(const Vector<N, T>& org, const P&... sizes);
        ParallelotopeAA(const Vector<N, T>& org, const std::array<T, N>& sizes);
        ParallelotopeAA(const Vector<N, T>& min, const Vector<N, T>& max);

        Constraints<N, T, 2 * N, 0> constraints() const;

        bool inside(const Vector<N, T>& p) const;

        std::optional<T> intersect(const Ray<N, T>& r) const;
        std::optional<T> intersect_farthest(const Ray<N, T>& r) const;
        std::optional<T> intersect_volume(const Ray<N, T>& r) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<ParallelotopeAA<N, T>, DIVISIONS> binary_division() const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> vertex_ridges() const;

        T length() const;

        Vector<N, T> org() const;
        Vector<N, T> e(unsigned n) const;

        Vector<N, T> min() const;
        Vector<N, T> max() const;
};

template <std::size_t N, typename T>
template <typename... P>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& org, const P&... sizes)
        : ParallelotopeAA(org, std::array<T, N>{sizes...})
{
        static_assert((std::is_same_v<T, P> && ...));
        static_assert(sizeof...(P) == N);
}

template <std::size_t N, typename T>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& org, const std::array<T, N>& sizes)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!(sizes[i] > 0))
                {
                        error("Axis-aligned parallelotope sizes " + to_string(sizes));
                }
        }
        for (unsigned i = 0; i < N; ++i)
        {
                m_planes[i].d1 = org[i];
                m_planes[i].d2 = org[i] + sizes[i];
        }
}

template <std::size_t N, typename T>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& min, const Vector<N, T>& max)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!(max[i] - min[i] > 0))
                {
                        error("Axis-aligned parallelotope min " + to_string(min) + ", max " + to_string(max));
                }
        }
        for (unsigned i = 0; i < N; ++i)
        {
                m_planes[i].d1 = min[i];
                m_planes[i].d2 = max[i];
        }
}

template <std::size_t N, typename T>
T ParallelotopeAA<N, T>::size(unsigned i) const
{
        return m_planes[i].d2 - m_planes[i].d1;
}

// Неравенства в виде b + a * x >= 0, задающие множество точек параллелотопа.
template <std::size_t N, typename T>
Constraints<N, T, 2 * N, 0> ParallelotopeAA<N, T>::constraints() const
{
        Constraints<N, T, 2 * N, 0> result;

        // Плоскости n * x - d имеют перпендикуляр с направлением наружу.
        // Направление внутрь -n * x + d или d + -(n * x), тогда условие
        // для точек параллелотопа d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                result.c[c_i].a = NORMALS_POSITIVE[i];
                result.c[c_i].b = -m_planes[i].d1;

                result.c[c_i + 1].a = NORMALS_NEGATIVE[i];
                result.c[c_i + 1].b = m_planes[i].d2;
        }

        return result;
}

template <std::size_t N, typename T>
bool ParallelotopeAA<N, T>::intersect_impl(const Ray<N, T>& r, T* first, T* second) const
{
        T f_max = limits<T>::lowest();
        T b_min = limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                T s = r.dir()[i]; // dot(r.dir(), NORMALS_POSITIVE[i])
                if (s == 0)
                {
                        T d = r.org()[i]; // dot(r.org(), NORMALS_POSITIVE[i])
                        if (d < m_planes[i].d1 || d > m_planes[i].d2)
                        {
                                // параллельно плоскостям и снаружи
                                return false;
                        }
                        // внутри плоскостей
                        continue;
                }

                T d = r.org()[i]; // dot(r.org(), NORMALS_POSITIVE[i])
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

template <std::size_t N, typename T>
std::optional<T> ParallelotopeAA<N, T>::intersect(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return (first > 0) ? first : second;
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
std::optional<T> ParallelotopeAA<N, T>::intersect_farthest(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return second;
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
std::optional<T> ParallelotopeAA<N, T>::intersect_volume(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return std::max(first, T(0));
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::normal(const Vector<N, T>& p) const
{
        // К какой плоскости точка ближе, такой и перпендикуляр в точке

        T min = limits<T>::max();

        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                T l;

                l = std::abs(p[i] - m_planes[i].d1);
                if (l < min)
                {
                        min = l;
                        n = NORMALS_NEGATIVE[i];
                }

                l = std::abs(p[i] - m_planes[i].d2);
                if (l < min)
                {
                        min = l;
                        n = NORMALS_POSITIVE[i];
                }
        }

        ASSERT(min < limits<T>::max());

        return n;
}

template <std::size_t N, typename T>
bool ParallelotopeAA<N, T>::inside(const Vector<N, T>& p) const
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (p[i] < m_planes[i].d1)
                {
                        return false;
                }

                if (p[i] > m_planes[i].d2)
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void ParallelotopeAA<N, T>::binary_division_impl(std::array<Planes, N>* p, const Vector<N, T>& middle_d, const F& f)
        const
{
        if constexpr (INDEX >= 0)
        {
                (*p)[INDEX].d1 = m_planes[INDEX].d1;
                (*p)[INDEX].d2 = middle_d[INDEX];
                binary_division_impl<INDEX - 1>(p, middle_d, f);
                (*p)[INDEX].d1 = middle_d[INDEX];
                (*p)[INDEX].d2 = m_planes[INDEX].d2;
                binary_division_impl<INDEX - 1>(p, middle_d, f);
        }
        else
        {
                f();
        }
}

template <std::size_t N, typename T>
std::array<ParallelotopeAA<N, T>, ParallelotopeAA<N, T>::DIVISIONS> ParallelotopeAA<N, T>::binary_division() const
{
        std::array<ParallelotopeAA, DIVISIONS> result;

        Vector<N, T> middle_d;
        for (unsigned i = 0; i < N; ++i)
        {
                middle_d[i] = (m_planes[i].d1 + m_planes[i].d2) / static_cast<T>(2);
        }

        unsigned count = 0;
        std::array<Planes, N> p;
        auto f = [&count, &result, &p]()
        {
                ASSERT(count < result.size());
                result[count++].m_planes = p;
        };

        binary_division_impl<N - 1>(&p, middle_d, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void ParallelotopeAA<N, T>::vertices_impl(Vector<N, T>* p, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                (*p)[INDEX] = m_planes[INDEX].d1;
                vertices_impl<INDEX - 1>(p, f);
                (*p)[INDEX] = m_planes[INDEX].d2;
                vertices_impl<INDEX - 1>(p, f);
        }
        else
        {
                f();
        }
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, ParallelotopeAA<N, T>::VERTEX_COUNT> ParallelotopeAA<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;
        Vector<N, T> p;
        auto f = [&count, &result, &p]()
        {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 1>(&p, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
std::array<std::array<Vector<N, T>, 2>, ParallelotopeAA<N, T>::VERTEX_RIDGE_COUNT> ParallelotopeAA<N, T>::
        vertex_ridges() const
{
        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> result;

        std::array<Vector<N, T>, N> vectors;
        for (unsigned i = 0; i < N; ++i)
        {
                vectors[i] = parallelotope_aa_implementation::index_vector<N, T>(i, m_planes[i].d2 - m_planes[i].d1);
        }

        unsigned count = 0;
        Vector<N, T> p;
        auto f = [this, &count, &result, &p, &vectors]()
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (p[i] == m_planes[i].d1)
                        {
                                ASSERT(count < result.size());
                                result[count][0] = p;
                                result[count][1] = vectors[i];
                                ++count;
                        }
                }
        };

        vertices_impl<N - 1>(&p, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
T ParallelotopeAA<N, T>::length() const
{
        Vector<N, T> s;
        for (unsigned i = 0; i < N; ++i)
        {
                s[i] = m_planes[i].d2 - m_planes[i].d1;
        }
        return s.norm();
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::org() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = m_planes[i].d1;
        }
        return v;
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::e(unsigned n) const
{
        ASSERT(n < N);
        return parallelotope_aa_implementation::index_vector<N, T>(n, size(n));
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::min() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = m_planes[i].d1;
        }
        return v;
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::max() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = m_planes[i].d2;
        }
        return v;
}
}

namespace ns
{
template <std::size_t N, typename T>
std::string to_string(const painter::ParallelotopeAA<N, T>& p)
{
        std::string s;
        s += "org = " + to_string(p.org()) + "\n";
        for (unsigned i = 0; i < N; ++i)
        {
                s += "edge[" + to_string(i) + "] = " + to_string(p.e(i)) + ((i < N - 1) ? "\n" : "");
        }
        return s;
}
}
