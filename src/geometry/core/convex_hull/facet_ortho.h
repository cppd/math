/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/set_mpz.h>
#include <src/com/type/concept.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>

#include <gmp.h>
#include <gmpxx.h>

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace ns::geometry::core::convex_hull
{
namespace facet_ortho_implementation
{
template <std::size_t N, typename T>
void reduce(numerical::Vector<N, T>* const)
{
        static_assert(!std::is_class_v<T>);
}

template <std::size_t N>
void reduce(numerical::Vector<N, mpz_class>* const v)
{
        static_assert(N >= 2);

        thread_local mpz_class gcd;

        mpz_gcd(gcd.get_mpz_t(), (*v)[0].get_mpz_t(), (*v)[1].get_mpz_t());
        for (std::size_t i = 2; i < N && gcd != 1; ++i)
        {
                mpz_gcd(gcd.get_mpz_t(), gcd.get_mpz_t(), (*v)[i].get_mpz_t());
        }

        if (gcd <= 1)
        {
                return;
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                mpz_divexact((*v)[i].get_mpz_t(), (*v)[i].get_mpz_t(), gcd.get_mpz_t());
        }
}

//

template <std::size_t N, typename T>
void negate(numerical::Vector<N, T>* const v)
{
        static_assert(!std::is_class_v<T>);

        for (std::size_t i = 0; i < N; ++i)
        {
                (*v)[i] = -(*v)[i];
        }
}

template <std::size_t N>
void negate(numerical::Vector<N, mpz_class>* const v)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                mpz_neg((*v)[i].get_mpz_t(), (*v)[i].get_mpz_t());
        }
}

//

template <std::size_t N, typename T>
[[nodiscard]] bool are_opposite(const numerical::Vector<N, T>& v1, const numerical::Vector<N, T>& v2)
{
        static_assert(!std::is_class_v<T>);

        for (std::size_t i = 0; i < N; ++i)
        {
                if ((v1[i] > 0 && v2[i] < 0) || (v1[i] < 0 && v2[i] > 0))
                {
                        return true;
                }
        }
        return false;
}

template <std::size_t N>
[[nodiscard]] bool are_opposite(const numerical::Vector<N, mpz_class>& v1, const numerical::Vector<N, mpz_class>& v2)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const int sgn1 = mpz_sgn(v1[i].get_mpz_t());
                const int sgn2 = mpz_sgn(v2[i].get_mpz_t());
                if ((sgn1 > 0 && sgn2 < 0) || (sgn1 < 0 && sgn2 > 0))
                {
                        return true;
                }
        }
        return false;
}

//

template <std::size_t N, typename T>
[[nodiscard]] bool last_coord_is_negative(const numerical::Vector<N, T>& v)
{
        static_assert(!std::is_class_v<T>);

        return v[N - 1] < 0;
}

template <std::size_t N>
[[nodiscard]] bool last_coord_is_negative(const numerical::Vector<N, mpz_class>& v)
{
        return mpz_sgn(v[N - 1].get_mpz_t()) < 0;
}

//

template <typename Result, std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, Result> normalize(const numerical::Vector<N, T>& v)
{
        static_assert(!std::is_class_v<T>);
        static_assert(std::is_floating_point_v<Result>);

        return to_vector<Result>(v).normalized();
}

template <typename Result, std::size_t N>
[[nodiscard]] numerical::Vector<N, Result> normalize(const numerical::Vector<N, mpz_class>& v)
{
        static_assert(std::is_same_v<Result, float> || std::is_same_v<Result, double>);

        static constexpr int FLOAT_BIT_PRECISION = 128;

        const mpf_class& length = [&v]
        {
                thread_local mpz_class d;
                mpz_mul(d.get_mpz_t(), v[0].get_mpz_t(), v[0].get_mpz_t());
                for (std::size_t i = 1; i < N; ++i)
                {
                        mpz_addmul(d.get_mpz_t(), v[i].get_mpz_t(), v[i].get_mpz_t());
                }

                thread_local mpf_class res(0, FLOAT_BIT_PRECISION);
                res = d;
                mpf_sqrt(res.get_mpf_t(), res.get_mpf_t());
                return res;
        }();

        numerical::Vector<N, Result> res;
        thread_local mpf_class v_mpf(0, FLOAT_BIT_PRECISION);
        for (std::size_t i = 0; i < N; ++i)
        {
                v_mpf = v[i];
                v_mpf /= length;
                res[i] = v_mpf.get_d();
        }
        return res;
}

//

template <std::size_t N, typename DataType, typename ComputeType>
[[nodiscard]] ComputeType dot_product_sign(
        const numerical::Vector<N, ComputeType>& v,
        const std::vector<numerical::Vector<N, DataType>>& points,
        const int from_index,
        const int to_index)
{
        static_assert(!std::is_class_v<DataType> && !std::is_class_v<ComputeType>);
        static_assert(std::is_same_v<ComputeType, std::common_type_t<ComputeType, DataType>>);

        const numerical::Vector<N, DataType>& from = points[from_index];
        const numerical::Vector<N, DataType>& to = points[to_index];

        ComputeType res = v[0] * (to[0] - from[0]);
        for (std::size_t i = 1; i < N; ++i)
        {
                res += v[i] * (to[i] - from[i]);
        }
        return res;
}

template <std::size_t N, typename DataType>
[[nodiscard]] int dot_product_sign(
        const numerical::Vector<N, mpz_class>& v,
        const std::vector<numerical::Vector<N, DataType>>& points,
        const int from_index,
        const int to_index)
{
        static_assert(!std::is_class_v<DataType>);

        thread_local mpz_class d;
        thread_local mpz_class w;

        const numerical::Vector<N, DataType>& from = points[from_index];
        const numerical::Vector<N, DataType>& to = points[to_index];

        set_mpz(&w, to[0] - from[0]);
        mpz_mul(d.get_mpz_t(), v[0].get_mpz_t(), w.get_mpz_t());
        for (std::size_t i = 1; i < N; ++i)
        {
                set_mpz(&w, to[i] - from[i]);
                mpz_addmul(d.get_mpz_t(), v[i].get_mpz_t(), w.get_mpz_t());
        }

        return mpz_sgn(d.get_mpz_t());
}

template <std::size_t N>
[[nodiscard]] int dot_product_sign(
        const numerical::Vector<N, mpz_class>& v,
        const std::vector<numerical::Vector<N, mpz_class>>& points,
        const int from_index,
        const int to_index)
{
        thread_local mpz_class d;
        thread_local mpz_class w;

        const numerical::Vector<N, mpz_class>& from = points[from_index];
        const numerical::Vector<N, mpz_class>& to = points[to_index];

        mpz_sub(w.get_mpz_t(), to[0].get_mpz_t(), from[0].get_mpz_t());
        mpz_mul(d.get_mpz_t(), v[0].get_mpz_t(), w.get_mpz_t());
        for (std::size_t i = 1; i < N; ++i)
        {
                mpz_sub(w.get_mpz_t(), to[i].get_mpz_t(), from[i].get_mpz_t());
                mpz_addmul(d.get_mpz_t(), v[i].get_mpz_t(), w.get_mpz_t());
        }

        return mpz_sgn(d.get_mpz_t());
}
}

template <std::size_t N, typename DataType, typename ComputeType>
class FacetOrtho final
{
        static_assert(Integral<DataType> && Integral<ComputeType>);
        static_assert(Signed<DataType> && Signed<ComputeType>);

        static constexpr bool REDUCE = false;

        numerical::Vector<N, ComputeType> ortho_;

        template <bool USE_DIRECTION_FACET>
        FacetOrtho(
                std::bool_constant<USE_DIRECTION_FACET>,
                const std::vector<numerical::Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const FacetOrtho* const direction_facet)
                : ortho_(numerical::orthogonal_complement<ComputeType>(points, vertices))
        {
                namespace impl = facet_ortho_implementation;

                ASSERT(!ortho_.is_zero());

                if constexpr (REDUCE)
                {
                        impl::reduce(&ortho_);
                }

                const auto v = impl::dot_product_sign(ortho_, points, vertices[0], direction_point);

                if (v < 0)
                {
                        // direction point is invisible, ortho is directed outside
                        return;
                }

                if (v > 0)
                {
                        // direction point is visible, change ortho direction
                        impl::negate(&ortho_);
                        return;
                }

                // direction point is on the facet plane

                if constexpr (USE_DIRECTION_FACET)
                {
                        if (impl::are_opposite(ortho_, direction_facet->ortho_))
                        {
                                impl::negate(&ortho_);
                        }
                        return;
                }

                error("Direction point is on the facet plane");
        }

public:
        FacetOrtho(
                const std::vector<numerical::Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point,
                const FacetOrtho& direction_facet)
                : FacetOrtho(std::bool_constant<true>(), points, vertices, direction_point, &direction_facet)
        {
        }

        FacetOrtho(
                const std::vector<numerical::Vector<N, DataType>>& points,
                const std::array<int, N>& vertices,
                const int direction_point)
                : FacetOrtho(std::bool_constant<false>(), points, vertices, direction_point, nullptr)
        {
        }

        [[nodiscard]] auto dot_product_sign(
                const std::vector<numerical::Vector<N, DataType>>& points,
                const int from_index,
                const int to_index) const
        {
                return facet_ortho_implementation::dot_product_sign(ortho_, points, from_index, to_index);
        }

        template <typename T>
        [[nodiscard]] numerical::Vector<N, T> to_floating_point() const
        {
                return facet_ortho_implementation::normalize<T>(ortho_);
        }

        [[nodiscard]] bool last_coord_is_negative() const
        {
                return facet_ortho_implementation::last_coord_is_negative(ortho_);
        }
};
}
