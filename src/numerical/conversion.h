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

#include "vector.h"

#include <src/com/set_mpz.h>

#include <gmp.h>
#include <gmpxx.h>

#include <cstddef>
#include <type_traits>

namespace ns::numerical
{
template <std::size_t N, typename Dst, typename Src>
void set_vector(Vector<N, Dst>* const dst, const Vector<N, Src>& to, const Vector<N, Src>& from)
{
        static_assert(!std::is_same_v<mpz_class, std::remove_cv_t<Dst>>);
        static_assert(!std::is_same_v<mpz_class, std::remove_cv_t<Src>>);

        for (std::size_t i = 0; i < N; ++i)
        {
                (*dst)[i] = to[i] - from[i];
        }
}

template <std::size_t N, typename Src>
void set_vector(Vector<N, mpz_class>* const v, const Vector<N, Src>& to, const Vector<N, Src>& from)
{
        static_assert(!std::is_same_v<mpz_class, std::remove_cv_t<Src>>);

        for (std::size_t i = 0; i < N; ++i)
        {
                set_mpz(&(*v)[i], to[i] - from[i]);
        }
}

template <std::size_t N>
void set_vector(Vector<N, mpz_class>* const v, const Vector<N, mpz_class>& to, const Vector<N, mpz_class>& from)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                mpz_sub((*v)[i]->get_mpz_t(), to[i].get_mpz_t(), from[i].get_mpz_t());
        }
}
}
