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

        // dot(ortho, vector from facet to point)
        [[nodiscard]] ComputeType visible(
                const std::vector<Vector<N, DataType>>& points,
                const int facet_point_index,
                const int point_index) const
        {
                const Vector<N, DataType>& facet_point = points[facet_point_index];
                const Vector<N, DataType>& point = points[point_index];

                ComputeType d = ortho_[0] * (point[0] - facet_point[0]);
                for (unsigned n = 1; n < N; ++n)
                {
                        d += ortho_[n] * (point[n] - facet_point[n]);
                }
                return d;
        }

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

                const ComputeType v = visible(points, vertices[0], direction_point);

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

        [[nodiscard]] bool visible_from_point(
                const std::vector<Vector<N, DataType>>& points,
                const int facet_point_index,
                const int point_index) const
        {
                // strictly greater than 0
                return visible(points, facet_point_index, point_index) > 0;
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

        static void reduce(Vector<N, mpz_class>* const ortho)
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

        static void negate(Vector<N, mpz_class>* const v)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        mpz_neg((*v)[n].get_mpz_t(), (*v)[n].get_mpz_t());
                }
        }

        static void dot(mpz_class* const d, const Vector<N, mpz_class>& v1, const Vector<N, mpz_class>& v2)
        {
                mpz_mul(d->get_mpz_t(), v1[0].get_mpz_t(), v2[0].get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_addmul(d->get_mpz_t(), v1[n].get_mpz_t(), v2[n].get_mpz_t());
                }
        }

        [[nodiscard]] static Vector<N, double> normalized_double_vector(const Vector<N, mpz_class>& mpz_vec)
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

        // sign of dot(ortho, vector from facet to point)
        [[nodiscard]] int visible(
                const std::vector<Vector<N, DataType>>& points,
                const int facet_point_index,
                const int point_index) const
        {
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, DataType>& facet_point = points[facet_point_index];
                const Vector<N, DataType>& point = points[point_index];

                set_mpz(&to_point, point[0] - facet_point[0]);
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        set_mpz(&to_point, point[n] - facet_point[n]);
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), to_point.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

        // sign of dot(ortho, vector from facet to point)
        [[nodiscard]] int visible(
                const std::vector<Vector<N, mpz_class>>& points,
                const int facet_point_index,
                const int point_index) const
        {
                thread_local mpz_class d;
                thread_local mpz_class to_point;

                const Vector<N, mpz_class>& facet_point = points[facet_point_index];
                const Vector<N, mpz_class>& point = points[point_index];

                mpz_sub(to_point.get_mpz_t(), point[0].get_mpz_t(), facet_point[0].get_mpz_t());
                mpz_mul(d.get_mpz_t(), ortho_[0].get_mpz_t(), to_point.get_mpz_t());
                for (unsigned n = 1; n < N; ++n)
                {
                        mpz_sub(to_point.get_mpz_t(), point[n].get_mpz_t(), facet_point[n].get_mpz_t());
                        mpz_addmul(d.get_mpz_t(), ortho_[n].get_mpz_t(), to_point.get_mpz_t());
                }

                return mpz_sgn(d.get_mpz_t());
        }

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

                const int v = visible(points, vertices[0], direction_point);

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

        [[nodiscard]] bool visible_from_point(
                const std::vector<Vector<N, DataType>>& points,
                const int facet_point_index,
                const int point_index) const
        {
                // strictly greater than 0
                return visible(points, facet_point_index, point_index) > 0;
        }

        [[nodiscard]] Vector<N, double> double_ortho() const
        {
                return normalized_double_vector(ortho_);
        }

        [[nodiscard]] bool last_ortho_coord_is_negative() const
        {
                return mpz_sgn(ortho_[N - 1].get_mpz_t()) < 0;
        }
};
}
