/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/com/type/concept.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>

#include <array>
#include <type_traits>
#include <vector>

namespace ns::geometry::convex_hull
{
template <std::size_t N, typename DataType, typename ComputeType>
class IntegerFacet final
{
        static_assert(!std::is_class_v<DataType> && !std::is_class_v<ComputeType>);
        static_assert(Integral<DataType> && Integral<ComputeType>);
        static_assert(Signed<DataType> && Signed<ComputeType>);

        template <typename T>
        static void negate(Vector<N, T>* const v)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        (*v)[n] = -(*v)[n];
                }
        }

        template <typename T>
        [[nodiscard]] static bool opposite_orthos(const Vector<N, T>& v1, const Vector<N, T>& v2)
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

        template <bool USE_DIRECTION_FACET>
        IntegerFacet(
                std::bool_constant<USE_DIRECTION_FACET>,
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const IntegerFacet* const direction_facet)
                : ortho_(numerical::orthogonal_complement<ComputeType>(points, vertices))
        {
                ASSERT(!ortho_.is_zero());

                const auto v = dot_product_sign(points, vertices[0], direction_point);

                if (v < 0)
                {
                        // direction point is invisible, ortho is directed outside
                        return;
                }

                if (v > 0)
                {
                        // direction point is visible, change ortho direction
                        negate(&ortho_);
                        return;
                }

                // direction point is on the facet plane

                if constexpr (USE_DIRECTION_FACET)
                {
                        if (opposite_orthos(ortho_, direction_facet->ortho_))
                        {
                                negate(&ortho_);
                        }
                        return;
                }

                error("Direction point is on the facet plane");
        }

public:
        IntegerFacet(
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const IntegerFacet& direction_facet)
                : IntegerFacet(
                        /*use direction facet*/ std::bool_constant<true>(),
                        points,
                        vertices,
                        direction_point,
                        &direction_facet)
        {
        }

        IntegerFacet(
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point)
                : IntegerFacet(
                        /*use direction facet*/ std::bool_constant<false>(),
                        points,
                        vertices,
                        direction_point,
                        nullptr)
        {
        }

        [[nodiscard]] ComputeType dot_product_sign(
                const std::vector<Vector<N, DataType>>& points,
                const int from_index,
                const int to_index) const
        {
                const Vector<N, DataType>& from = points[from_index];
                const Vector<N, DataType>& to = points[to_index];

                ComputeType d = ortho_[0] * (to[0] - from[0]);
                for (unsigned n = 1; n < N; ++n)
                {
                        d += ortho_[n] * (to[n] - from[n]);
                }
                return d;
        }

        [[nodiscard]] Vector<N, double> double_ortho() const
        {
                return to_vector<double>(ortho_).normalized();
        }

        [[nodiscard]] bool last_ortho_coord_is_negative() const
        {
                return ortho_[N - 1] < 0;
        }
};

template <std::size_t N, typename DataType>
class IntegerFacet<N, DataType, mpz_class> final
{
        static_assert(!std::is_class_v<DataType>);
        static_assert(Integral<DataType>);
        static_assert(Signed<DataType>);

        static constexpr bool REDUCE = false;

        static void reduce(Vector<N, mpz_class>* const v)
        {
                static_assert(N >= 2);

                thread_local mpz_class gcd;

                mpz_gcd(gcd.get_mpz_t(), (*v)[0].get_mpz_t(), (*v)[1].get_mpz_t());
                for (unsigned n = 2; n < N && gcd != 1; ++n)
                {
                        mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), (*v)[n].get_mpz_t());
                }

                if (gcd <= 1)
                {
                        return;
                }

                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_divexact((*v)[n].get_mpz_t(), (*v)[n].get_mpz_t(), gcd.get_mpz_t());
                }
        }

        static void negate(Vector<N, mpz_class>* const v)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_neg((*v)[n].get_mpz_t(), (*v)[n].get_mpz_t());
                }
        }

        static void length(mpf_class* const res, const Vector<N, mpz_class>& v)
        {
                thread_local mpz_class d;
                mpz_mul(d.get_mpz_t(), v[0].get_mpz_t(), v[0].get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_addmul(d.get_mpz_t(), v[n].get_mpz_t(), v[n].get_mpz_t());
                }
                *res = d;
                mpf_sqrt(res->get_mpf_t(), res->get_mpf_t());
        }

        template <typename T>
        [[nodiscard]] static Vector<N, T> normalize(const Vector<N, mpz_class>& v)
        {
                static constexpr int FLOAT_BIT_PRECISION = 128;

                thread_local mpf_class l(0, FLOAT_BIT_PRECISION);
                thread_local mpf_class v_float(0, FLOAT_BIT_PRECISION);

                length(&l, v);

                Vector<N, T> res;
                for (unsigned n = 0; n < N; ++n)
                {
                        v_float = v[n];
                        v_float /= l;
                        res[n] = v_float.get_d();
                }
                return res;
        }

        [[nodiscard]] static bool opposite_orthos(const Vector<N, mpz_class>& v1, const Vector<N, mpz_class>& v2)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        const int sgn1 = mpz_sgn(v1[n].get_mpz_t());
                        const int sgn2 = mpz_sgn(v2[n].get_mpz_t());
                        if ((sgn1 > 0 && sgn2 < 0) || (sgn1 < 0 && sgn2 > 0))
                        {
                                return true;
                        }
                }
                return false;
        }

        //

        Vector<N, mpz_class> ortho_;

        template <bool USE_DIRECTION_FACET>
        IntegerFacet(
                std::bool_constant<USE_DIRECTION_FACET>,
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const IntegerFacet* const direction_facet)
                : ortho_(numerical::orthogonal_complement<mpz_class>(points, vertices))
        {
                ASSERT(!ortho_.is_zero());

                if constexpr (REDUCE)
                {
                        reduce(&ortho_);
                }

                const auto v = dot_product_sign(points, vertices[0], direction_point);

                if (v < 0)
                {
                        // direction point is invisible, ortho is directed outside
                        return;
                }

                if (v > 0)
                {
                        // direction point is visible, change ortho direction
                        negate(&ortho_);
                        return;
                }

                // direction point is on the facet plane

                if constexpr (USE_DIRECTION_FACET)
                {
                        if (opposite_orthos(ortho_, direction_facet->ortho_))
                        {
                                negate(&ortho_);
                        }
                        return;
                }

                error("Direction point is on the facet plane");
        }

public:
        IntegerFacet(
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const IntegerFacet& direction_facet)
                : IntegerFacet(
                        /*use direction facet*/ std::bool_constant<true>(),
                        points,
                        vertices,
                        direction_point,
                        &direction_facet)
        {
        }

        IntegerFacet(
                const std::vector<Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point)
                : IntegerFacet(
                        /*use direction facet*/ std::bool_constant<false>(),
                        points,
                        vertices,
                        direction_point,
                        nullptr)
        {
        }

        [[nodiscard]] int dot_product_sign(
                const std::vector<Vector<N, DataType>>& points,
                const int from_index,
                const int to_index) const
        {
                thread_local mpz_class d;
                thread_local mpz_class v;

                const Vector<N, DataType>& from = points[from_index];
                const Vector<N, DataType>& to = points[to_index];

                set_mpz(&v, to[0] - from[0]);
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), v.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        set_mpz(&v, to[n] - from[n]);
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), v.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

        [[nodiscard]] int dot_product_sign(
                const std::vector<Vector<N, mpz_class>>& points,
                const int from_index,
                const int to_index) const
        {
                thread_local mpz_class d;
                thread_local mpz_class v;

                const Vector<N, mpz_class>& from = points[from_index];
                const Vector<N, mpz_class>& to = points[to_index];

                mpz_sub(v.get_mpz_t(), to[0].get_mpz_t(), from[0].get_mpz_t());
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), v.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_sub(v.get_mpz_t(), to[n].get_mpz_t(), from[n].get_mpz_t());
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), v.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

        [[nodiscard]] Vector<N, double> double_ortho() const
        {
                return normalize<double>(ortho_);
        }

        [[nodiscard]] bool last_ortho_coord_is_negative() const
        {
                return mpz_sgn(ortho_[N - 1].get_mpz_t()) < 0;
        }
};
}
