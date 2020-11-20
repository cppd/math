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
namespace parallelotope_implementation
{
template <size_t N, typename T, size_t... I>
std::array<Vector<N, T>, N> make_vectors_impl(const Vector<N, T>& d, std::integer_sequence<size_t, I...>)
{
        static_assert(N == sizeof...(I));
        // Заполнение диагонали матрицы NxN значениями из d, остальные элементы 0
        std::array<Vector<N, T>, N> vectors{(static_cast<void>(I), Vector<N, T>(0))...};
        ((vectors[I][I] = d[I]), ...);
        return vectors;
}

template <size_t N, typename T, size_t... I>
std::array<Vector<N, T>, N> make_vectors(const Vector<N, T>& d)
{
        return make_vectors_impl(d, std::make_integer_sequence<size_t, N>());
}
}

template <size_t N, typename T>
class Parallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static_assert(N <= 27);

        // Количество объектов после деления по каждому измерению
        static constexpr int DIVISIONS = 1 << N;

        static constexpr int DIAGONAL_COUNT = 1 << (N - 1);
        static constexpr int VERTEX_COUNT = 1 << N;

        // Количество вершин 2 ^ N умножить на количество измерений N у каждой вершины
        // и для уникальности разделить на 2 = ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int VERTEX_RIDGE_COUNT = (1 << (N - 1)) * N;

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

        template <int INDEX, typename F>
        void binary_division_impl(
                const Vector<N, T>& org,
                Vector<N, T>* d1,
                Vector<N, T>* d2,
                const std::array<Vector<N, T>, N>& half_vectors,
                const Vector<N, T>& middle_d,
                const F& f) const;

        template <int INDEX, typename F>
        void vertices_impl(const Vector<N, T>& p, const F& f) const;

        template <int INDEX, typename F>
        void vertex_ridges_impl(const Vector<N, T>& p, std::array<bool, N>* dimensions, const F& f) const;

        template <int INDEX, typename F>
        void length_impl(const Vector<N, T>& sum, const F& f) const;

public:
        static constexpr size_t SPACE_DIMENSION = N;
        static constexpr size_t SHAPE_DIMENSION = N;

        using DataType = T;

        Parallelotope() = default;

        template <typename... P>
        explicit Parallelotope(const Vector<N, T>& org, const P&... vectors);
        Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max);

        Constraints<N, T, 2 * N, 0> constraints() const;

        bool inside(const Vector<N, T>& p) const;

        std::optional<T> intersect(const Ray<N, T>& r) const;
        std::optional<T> intersect_farthest(const Ray<N, T>& r) const;
        std::optional<T> intersect_volume(const Ray<N, T>& r) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<Parallelotope<N, T>, DIVISIONS> binary_division() const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> vertex_ridges() const;

        T length() const;

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
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max)
{
        set_data(min, parallelotope_implementation::make_vectors(max - min));
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
Constraints<N, T, 2 * N, 0> Parallelotope<N, T>::constraints() const
{
        Constraints<N, T, 2 * N, 0> result;

        // Плоскости n * x - d имеют перпендикуляр с направлением наружу.
        // Направление внутрь -n * x + d или d + -(n * x), тогда условие
        // для точек параллелотопа d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                result.c[c_i].a = m_planes[i].n;
                result.c[c_i].b = -m_planes[i].d1;

                result.c[c_i + 1].a = -m_planes[i].n;
                result.c[c_i + 1].b = m_planes[i].d2;
        }

        return result;
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
std::optional<T> Parallelotope<N, T>::intersect(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return (first > 0) ? first : second;
        }
        return std::nullopt;
}

template <size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_farthest(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return second;
        }
        return std::nullopt;
}

template <size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_volume(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return std::max(first, T(0));
        }
        return std::nullopt;
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
template <int INDEX, typename F>
void Parallelotope<N, T>::binary_division_impl(
        const Vector<N, T>& org,
        Vector<N, T>* d1,
        Vector<N, T>* d2,
        const std::array<Vector<N, T>, N>& half_vectors,
        const Vector<N, T>& middle_d,
        const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                (*d1)[INDEX] = m_planes[INDEX].d1;
                (*d2)[INDEX] = middle_d[INDEX];
                binary_division_impl<INDEX - 1>(org, d1, d2, half_vectors, middle_d, f);
                (*d1)[INDEX] = middle_d[INDEX];
                (*d2)[INDEX] = m_planes[INDEX].d2;
                binary_division_impl<INDEX - 1>(org + half_vectors[INDEX], d1, d2, half_vectors, middle_d, f);
        }
        else
        {
                f(org);
        }
}

template <size_t N, typename T>
std::array<Parallelotope<N, T>, Parallelotope<N, T>::DIVISIONS> Parallelotope<N, T>::binary_division() const
{
        std::array<Parallelotope, DIVISIONS> result;

        std::array<Vector<N, T>, N> half_vectors;
        Vector<N, T> middle_d;
        for (unsigned i = 0; i < N; ++i)
        {
                half_vectors[i] = m_vectors[i] / static_cast<T>(2);
                middle_d[i] = (m_planes[i].d2 + m_planes[i].d1) / static_cast<T>(2);
        }

        for (Parallelotope& p : result)
        {
                p.m_vectors = half_vectors;
                for (unsigned i = 0; i < N; ++i)
                {
                        p.m_planes[i].n = m_planes[i].n;
                }
        }

        Vector<N, T> d1;
        Vector<N, T> d2;
        unsigned count = 0;
        auto f = [&count, &result, &d1, &d2](const Vector<N, T>& org)
        {
                ASSERT(count < result.size());
                result[count].m_org = org;
                for (unsigned i = 0; i < N; ++i)
                {
                        result[count].m_planes[i].d1 = d1[i];
                        result[count].m_planes[i].d2 = d2[i];
                }
                ++count;
        };

        binary_division_impl<N - 1>(m_org, &d1, &d2, half_vectors, middle_d, f);

        ASSERT(count == result.size());

        return result;
}

template <size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::vertices_impl(const Vector<N, T>& p, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                vertices_impl<INDEX - 1>(p, f);
                vertices_impl<INDEX - 1>(p + m_vectors[INDEX], f);
        }
        else
        {
                f(p);
        }
}

template <size_t N, typename T>
std::array<Vector<N, T>, Parallelotope<N, T>::VERTEX_COUNT> Parallelotope<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;
        auto f = [&count, &result](const Vector<N, T>& p)
        {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 1>(m_org, f);

        ASSERT(count == result.size());

        return result;
}

template <size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::vertex_ridges_impl(const Vector<N, T>& p, std::array<bool, N>* dimensions, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                (*dimensions)[INDEX] = true;
                vertex_ridges_impl<INDEX - 1>(p, dimensions, f);

                (*dimensions)[INDEX] = false;
                vertex_ridges_impl<INDEX - 1>(p + m_vectors[INDEX], dimensions, f);
        }
        else
        {
                f(p);
        }
}

template <size_t N, typename T>
std::array<std::array<Vector<N, T>, 2>, Parallelotope<N, T>::VERTEX_RIDGE_COUNT> Parallelotope<N, T>::vertex_ridges()
        const
{
        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> result;

        unsigned count = 0;
        std::array<bool, N> dimensions;
        auto f = [this, &dimensions, &count, &result](const Vector<N, T>& p)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (dimensions[i])
                        {
                                ASSERT(count < result.size());
                                result[count][0] = p;
                                result[count][1] = m_vectors[i];
                                ++count;
                        }
                }
        };

        // Смещаться по каждому измерению для перехода к другой вершине.
        // Добавлять к массиву рёбер пары, состоящие из вершины и векторов
        // измерений, по которым не смещались для перехода к этой вершине.
        vertex_ridges_impl<N - 1>(m_org, &dimensions, f);

        ASSERT(count == result.size());

        return result;
}

template <size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::length_impl(const Vector<N, T>& sum, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                length_impl<INDEX - 1>(sum + m_vectors[INDEX], f);
                length_impl<INDEX - 1>(sum - m_vectors[INDEX], f);
        }
        else
        {
                f(sum);
        }
}

template <size_t N, typename T>
T Parallelotope<N, T>::length() const
{
        T max_squared = limits<T>::lowest();

        unsigned count = 0;

        auto f = [&max_squared, &count](const Vector<N, T>& d)
        {
                ++count;
                max_squared = std::max(max_squared, d.norm_squared());
        };

        // Перебрать все диагонали одной из граней параллелотопа с учётом их направления.
        // Количество таких диагоналей равно 2 ^ (N - 1). Добавляя к каждой такой
        // диагонали оставшееся измерение, получаются все диагонали целого параллелотопа.
        // Одно из измерений не меняется, остальные к нему прибавляются и вычитаются.
        constexpr int LAST_INDEX = N - 1;
        length_impl<LAST_INDEX - 1>(m_vectors[LAST_INDEX], f);

        ASSERT(count == DIAGONAL_COUNT);

        return std::sqrt(max_squared);
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
