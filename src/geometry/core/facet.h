/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/mpz.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/com/type/trait.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <array>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename Derived, template <typename> typename FacetIter>
class FacetBase
{
        static_assert(N > 1);

        // Вершины должны быть отсортированы, чтобы были одинаковые одинаковые последовательности индексов у одинаковых ребер.
        const std::array<int, N> m_indices;

        std::vector<int> m_conflict_points;

        // Итератор на саму грань, чтобы удалять грань из списка без поиска в этом списке
        FacetIter<Derived> m_list_iter;

        // Указатели на другие грани, соответствующие вершинам
        std::array<Derived*, N> m_links;

        // Используется в алгоритме для упрощения поиска и к состоянию грани не имеет отношения, поэтому mutable
        mutable bool m_marked_as_visible = false;

protected:
        ~FacetBase() = default;

public:
        explicit FacetBase(std::array<int, N>&& vertices) : m_indices(sort(std::move(vertices)))
        {
        }

        const std::array<int, N>& vertices() const
        {
                return m_indices;
        }

        int find_index_for_point(int point) const
        {
                for (unsigned r = 0; r < N; ++r)
                {
                        if (m_indices[r] == point)
                        {
                                return r;
                        }
                }
                error("local index not found for point " + to_string(point));
        }

        void add_conflict_point(int point)
        {
                m_conflict_points.push_back(point);
        }
        const std::vector<int>& conflict_points() const
        {
                return m_conflict_points;
        }

        void set_iter(FacetIter<Derived> iter)
        {
                m_list_iter = iter;
        }
        FacetIter<Derived> iter() const
        {
                return m_list_iter;
        }

        void set_link(unsigned i, Derived* facet)
        {
                ASSERT(i < N);
                m_links[i] = facet;
        }
        Derived* link(unsigned i) const
        {
                ASSERT(i < N);
                return m_links[i];
        }
        unsigned find_link_index(const Derived* facet)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (m_links[i] == facet)
                        {
                                return i;
                        }
                }
                error("link index not found for facet");
        }

        void mark_as_visible() const
        {
                m_marked_as_visible = true;
        }
        bool marked_as_visible() const
        {
                return m_marked_as_visible;
        }
};

template <std::size_t N, typename DataType, typename ComputeType, template <typename> typename FacetIter>
class FacetInteger final : public FacetBase<N, FacetInteger<N, DataType, ComputeType, FacetIter>, FacetIter>
{
        using Base = FacetBase<N, FacetInteger, FacetIter>;

        static_assert(is_native_integral<DataType> && is_native_integral<ComputeType>);
        static_assert(is_signed<DataType> && is_signed<ComputeType>);

        // Перпендикуляр к грани (вектор из одномерного ортогонального дополнения грани).
        Vector<N, ComputeType> m_ortho;

        template <typename T>
        static void negate(Vector<N, T>* v)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        (*v)[n] = -(*v)[n];
                }
        }

        template <typename T>
        static bool opposite_orthos(const Vector<N, T>& v1, const Vector<N, T>& v2)
        {
                //Нельзя брать скалярное произведение этих двух перпендикуляров, так как может быть переполнение
                for (unsigned n = 0; n < N; ++n)
                {
                        if (((v1[n] > 0) && (v2[n] < 0)) || ((v1[n] < 0) && (v2[n] > 0)))
                        {
                                return true;
                        }
                }
                return false;
        }

        // Скалярное произведение перпендикуляра и вектора от одной из вершин грани к точке
        // Больше 0 видимая, меньше 0 невидимая, 0 в одной плоскости
        ComputeType visible(const std::vector<Vector<N, DataType>>& points, int p) const
        {
                const Vector<N, DataType>& facet_point = points[Base::vertices()[0]];
                const Vector<N, DataType>& point = points[p];

                ComputeType d = m_ortho[0] * (point[0] - facet_point[0]);
                for (unsigned n = 1; n < N; ++n)
                {
                        d += m_ortho[n] * (point[n] - facet_point[n]);
                }

                return d;
        }

public:
        FacetInteger(
                const std::vector<Vector<N, DataType>>& points,
                std::array<int, N>&& vertices,
                int convex_hull_point,
                const FacetInteger* convex_hull_facet)
                : Base(std::move(vertices))
        {
                m_ortho = numerical::orthogonal_complement<N, DataType, ComputeType>(points, Base::vertices());

                ASSERT(!m_ortho.is_zero());

                ComputeType v = visible(points, convex_hull_point);

                if (v < 0)
                {
                        // Точка оболочки невидимая, значит перпендикуляр направлен наружу от оболочки
                        return;
                }
                if (v > 0)
                {
                        // Точка оболочки видимая, значит поменять направление перпендикуляра наружу от оболочки
                        negate(&m_ortho);
                        return;
                }
                //   Точка и имеющаяся грань горизонта находятся в одной плоскости, значит их перпендикуляры
                // задать в одном направлении.
                //   convex_hull_facet == nullptr при создании начальной оболочки, где точки не должны быть в одной плоскости.
                ASSERT(convex_hull_facet != nullptr);
                if (opposite_orthos(m_ortho, convex_hull_facet->m_ortho))
                {
                        negate(&m_ortho);
                }
        }

        bool visible_from_point(const std::vector<Vector<N, DataType>>& points, int from_point) const
        {
                // Строго больше 0
                return visible(points, from_point) > 0;
        }

        Vector<N, double> double_ortho() const
        {
                return to_vector<double>(m_ortho).normalized();
        }

        bool last_ortho_coord_is_negative() const
        {
                return m_ortho[N - 1] < 0;
        }
};

template <std::size_t N, typename DataType, template <typename> typename FacetIter>
class FacetInteger<N, DataType, mpz_class, FacetIter> final
        : public FacetBase<N, FacetInteger<N, DataType, mpz_class, FacetIter>, FacetIter>
{
        static_assert(is_integral<DataType>);
        static_assert(is_signed<DataType>);

        using Base = FacetBase<N, FacetInteger, FacetIter>;

        // Перпендикуляр к грани (вектор из одномерного ортогонального дополнения грани).
        Vector<N, mpz_class> m_ortho;

#if 0
        static void reduce(Vector<N, mpz_class>* m_ortho)
        {
                thread_local mpz_class gcd;

                mpz_gcd(gcd.get_mpz_t(), (*m_ortho)[0].get_mpz_t(), (*m_ortho)[1].get_mpz_t());
                for (unsigned n = 2; n < N && gcd != 1; ++n)
                {
                        mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), (*m_ortho)[n].get_mpz_t());
                }
                if (gcd <= 1)
                {
                        return;
                }
                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_divexact((*m_ortho)[n].get_mpz_t(), (*m_ortho)[n].get_mpz_t(), gcd.get_mpz_t());
                }
        }
#endif

        static void negate(Vector<N, mpz_class>* v)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_neg((*v)[n].get_mpz_t(), (*v)[n].get_mpz_t());
                }
        }

        static void dot(mpz_class* d, const Vector<N, mpz_class>& v1, const Vector<N, mpz_class>& v2)
        {
                mpz_mul(d->get_mpz_t(), v1[0].get_mpz_t(), v2[0].get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_addmul(d->get_mpz_t(), v1[n].get_mpz_t(), v2[n].get_mpz_t());
                }
        }

        static Vector<N, double> normalized_double_vector(const Vector<N, mpz_class>& mpz_vec)
        {
                static constexpr int FLOAT_BIT_PRECISION = 128;

                thread_local mpz_class dot_product;
                thread_local mpf_class length(0, FLOAT_BIT_PRECISION);
                thread_local mpf_class ortho_coord(0, FLOAT_BIT_PRECISION);

                dot(&dot_product, mpz_vec, mpz_vec);

                length = dot_product;
                mpf_sqrt(length.get_mpf_t(), length.get_mpf_t());

                Vector<N, double> res;
                for (unsigned n = 0; n < N; ++n)
                {
                        ortho_coord = mpz_vec[n];
                        ortho_coord /= length;
                        res[n] = ortho_coord.get_d();
                }

                return res;
        }

        static bool opposite_orthos(const Vector<N, mpz_class>& v1, const Vector<N, mpz_class>& v2)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        int sgn1 = mpz_sgn(v1[n].get_mpz_t());
                        int sgn2 = mpz_sgn(v2[n].get_mpz_t());
                        if ((sgn1 > 0 && sgn2 < 0) || (sgn1 < 0 && sgn2 > 0))
                        {
                                return true;
                        }
                }
                return false;
        }

        // Знак скалярного произведения перпендикуляра и вектора от одной из вершин грани к точке
        // Больше 0 видимая, меньше 0 невидимая, 0 в одной плоскости
        int visible(const std::vector<Vector<N, DataType>>& points, int p) const
        {
                // thread_local - нужно избежать создания переменных mpz_class при каждом вызове функции
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, DataType>& facet_point = points[Base::vertices()[0]];
                const Vector<N, DataType>& point = points[p];

                mpz_from_any(&to_point, point[0] - facet_point[0]);
                mpz_mul(d.get_mpz_t(), m_ortho[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_from_any(&to_point, point[n] - facet_point[n]);
                        mpz_addmul(d.get_mpz_t(), m_ortho[n].get_mpz_t(), to_point.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }
        int visible(const std::vector<Vector<N, mpz_class>>& points, int p) const
        {
                // thread_local - нужно избежать создания переменных mpz_class при каждом вызове функции
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, mpz_class>& facet_point = points[Base::get_vertices()[0]];
                const Vector<N, mpz_class>& point = points[p];

                mpz_sub(to_point.get_mpz_t(), point[0].get_mpz_t(), facet_point[0].get_mpz_t());
                mpz_mul(d.get_mpz_t(), m_ortho[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_sub(to_point.get_mpz_t(), point[n].get_mpz_t(), facet_point[n].get_mpz_t());
                        mpz_addmul(d.get_mpz_t(), m_ortho[n].get_mpz_t(), to_point.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

public:
        FacetInteger(
                const std::vector<Vector<N, DataType>>& points,
                std::array<int, N>&& vertices,
                int convex_hull_point,
                const FacetInteger* convex_hull_facet)
                : Base(std::move(vertices))
        {
                m_ortho = numerical::orthogonal_complement<N, DataType, mpz_class>(points, Base::vertices());

                ASSERT(!m_ortho.is_zero());

#if 0
                // Сокращение перпендикуляра не даёт общего ускорения
                reduce(&m_ortho);
#endif

                int v = visible(points, convex_hull_point);

                if (v < 0)
                {
                        // Точка оболочки невидимая, значит перпендикуляр направлен наружу от оболочки
                        return;
                }
                if (v > 0)
                {
                        // Точка оболочки видимая, значит поменять направление перпендикуляра наружу от оболочки
                        negate(&m_ortho);
                        return;
                }
                //   Точка и имеющаяся грань горизонта находятся в одной плоскости, значит их перпендикуляры
                // задать в одном направлении.
                //   convex_hull_facet == nullptr при создании начальной оболочки, где точки не должны быть в одной плоскости.
                ASSERT(convex_hull_facet != nullptr);
                if (opposite_orthos(m_ortho, convex_hull_facet->m_ortho))
                {
                        negate(&m_ortho);
                }
        }

        bool visible_from_point(const std::vector<Vector<N, DataType>>& points, int from_point) const
        {
                // Строго больше 0
                return visible(points, from_point) > 0;
        }

        Vector<N, double> double_ortho() const
        {
                return normalized_double_vector(m_ortho);
        }

        bool last_ortho_coord_is_negative() const
        {
                return mpz_sgn(m_ortho[N - 1].get_mpz_t()) < 0;
        }
};
}
