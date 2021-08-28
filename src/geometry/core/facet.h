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
#include <src/com/type/concept.h>
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

        const std::array<int, N> vertices_;
        std::vector<int> conflict_points_;
        FacetIter<Derived> facet_iter_;
        std::array<Derived*, N> links_;
        mutable bool marked_as_visible_ = false;

protected:
        ~FacetBase() = default;

public:
        explicit FacetBase(std::array<int, N>&& vertices) : vertices_(sort(std::move(vertices)))
        {
        }

        const std::array<int, N>& vertices() const
        {
                return vertices_;
        }

        int find_index_for_point(int point) const
        {
                for (unsigned r = 0; r < N; ++r)
                {
                        if (vertices_[r] == point)
                        {
                                return r;
                        }
                }
                error("local index not found for point " + to_string(point));
        }

        void add_conflict_point(int point)
        {
                conflict_points_.push_back(point);
        }
        const std::vector<int>& conflict_points() const
        {
                return conflict_points_;
        }

        void set_iter(FacetIter<Derived> iter)
        {
                facet_iter_ = iter;
        }
        FacetIter<Derived> iter() const
        {
                return facet_iter_;
        }

        void set_link(unsigned i, Derived* facet)
        {
                ASSERT(i < N);
                links_[i] = facet;
        }
        Derived* link(unsigned i) const
        {
                ASSERT(i < N);
                return links_[i];
        }
        unsigned find_link_index(const Derived* facet)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (links_[i] == facet)
                        {
                                return i;
                        }
                }
                error("link index not found for facet");
        }

        void mark_as_visible() const
        {
                marked_as_visible_ = true;
        }
        bool marked_as_visible() const
        {
                return marked_as_visible_;
        }
};

template <std::size_t N, typename DataType, typename ComputeType, template <typename> typename FacetIter>
class FacetInteger final : public FacetBase<N, FacetInteger<N, DataType, ComputeType, FacetIter>, FacetIter>
{
        using Base = FacetBase<N, FacetInteger, FacetIter>;

        static_assert(!std::is_class_v<DataType> && !std::is_class_v<ComputeType>);
        static_assert(Integral<DataType> && Integral<ComputeType>);
        static_assert(Signed<DataType> && Signed<ComputeType>);

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
                for (unsigned n = 0; n < N; ++n)
                {
                        if ((v1[n] > 0 && v2[n] < 0) || (v1[n] < 0 && v2[n] > 0))
                        {
                                return true;
                        }
                }
                return false;
        }

        //

        Vector<N, ComputeType> ortho_;

        // dot(ortho, vector from facet to point)
        ComputeType visible(const std::vector<Vector<N, DataType>>& points, int p) const
        {
                const Vector<N, DataType>& facet_point = points[Base::vertices()[0]];
                const Vector<N, DataType>& point = points[p];

                ComputeType d = ortho_[0] * (point[0] - facet_point[0]);
                for (unsigned n = 1; n < N; ++n)
                {
                        d += ortho_[n] * (point[n] - facet_point[n]);
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
                ortho_ = numerical::orthogonal_complement<N, DataType, ComputeType>(points, Base::vertices());

                ASSERT(!ortho_.is_zero());

                ComputeType v = visible(points, convex_hull_point);

                if (v < 0)
                {
                        // A convex hull point is invisible, ortho is directed outside
                        return;
                }
                if (v > 0)
                {
                        // A convex hull point is visible, change ortho direction
                        negate(&ortho_);
                        return;
                }

                // A convex hull point is on the facet plane.
                // convex_hull_facet == nullptr when creating initial convex hull.
                ASSERT(convex_hull_facet != nullptr);
                if (opposite_orthos(ortho_, convex_hull_facet->ortho_))
                {
                        negate(&ortho_);
                }
        }

        bool visible_from_point(const std::vector<Vector<N, DataType>>& points, int from_point) const
        {
                // strictly greater than 0
                return visible(points, from_point) > 0;
        }

        Vector<N, double> double_ortho() const
        {
                return to_vector<double>(ortho_).normalized();
        }

        bool last_ortho_coord_is_negative() const
        {
                return ortho_[N - 1] < 0;
        }
};

template <std::size_t N, typename DataType, template <typename> typename FacetIter>
class FacetInteger<N, DataType, mpz_class, FacetIter> final
        : public FacetBase<N, FacetInteger<N, DataType, mpz_class, FacetIter>, FacetIter>
{
        static_assert(!std::is_class_v<DataType>);
        static_assert(Integral<DataType>);
        static_assert(Signed<DataType>);

        using Base = FacetBase<N, FacetInteger, FacetIter>;

        static void reduce(Vector<N, mpz_class>* ortho)
        {
                thread_local mpz_class gcd;

                mpz_gcd(gcd.get_mpz_t(), (*ortho)[0].get_mpz_t(), (*ortho)[1].get_mpz_t());
                for (unsigned n = 2; n < N && gcd != 1; ++n)
                {
                        mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), (*ortho)[n].get_mpz_t());
                }
                if (gcd <= 1)
                {
                        return;
                }
                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_divexact((*ortho)[n].get_mpz_t(), (*ortho)[n].get_mpz_t(), gcd.get_mpz_t());
                }
        }

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

        //

        Vector<N, mpz_class> ortho_;

        // sign of dot(ortho, vector from facet to point)
        int visible(const std::vector<Vector<N, DataType>>& points, int p) const
        {
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, DataType>& facet_point = points[Base::vertices()[0]];
                const Vector<N, DataType>& point = points[p];

                mpz_from_any(&to_point, point[0] - facet_point[0]);
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_from_any(&to_point, point[n] - facet_point[n]);
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), to_point.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

        // sign of dot(ortho, vector from facet to point)
        int visible(const std::vector<Vector<N, mpz_class>>& points, int p) const
        {
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, mpz_class>& facet_point = points[Base::get_vertices()[0]];
                const Vector<N, mpz_class>& point = points[p];

                mpz_sub(to_point.get_mpz_t(), point[0].get_mpz_t(), facet_point[0].get_mpz_t());
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_sub(to_point.get_mpz_t(), point[n].get_mpz_t(), facet_point[n].get_mpz_t());
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), to_point.get_mpz_t());
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
                ortho_ = numerical::orthogonal_complement<N, DataType, mpz_class>(points, Base::vertices());

                ASSERT(!ortho_.is_zero());

                if ((false))
                {
                        reduce(&ortho_);
                }

                int v = visible(points, convex_hull_point);

                if (v < 0)
                {
                        // a convex hull point is invisible, ortho is directed outside
                        return;
                }
                if (v > 0)
                {
                        // a convex hull point is visible, change ortho direction
                        negate(&ortho_);
                        return;
                }

                // A convex hull point is on the facet plane.
                // convex_hull_facet == nullptr when creating initial convex hull.
                ASSERT(convex_hull_facet != nullptr);
                if (opposite_orthos(ortho_, convex_hull_facet->ortho_))
                {
                        negate(&ortho_);
                }
        }

        bool visible_from_point(const std::vector<Vector<N, DataType>>& points, int from_point) const
        {
                // strictly greater than 0
                return visible(points, from_point) > 0;
        }

        Vector<N, double> double_ortho() const
        {
                return normalized_double_vector(ortho_);
        }

        bool last_ortho_coord_is_negative() const
        {
                return mpz_sgn(ortho_[N - 1].get_mpz_t()) < 0;
        }
};
}
