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

#include <gmpxx.h>
#include <type_traits>

namespace ns
{
namespace mpz_implementation
{
template <typename T>
void import(mpz_t mpz, const T v) requires(!std::is_class_v<T>)
{
        mpz_import(mpz, 1, -1, sizeof(v), 0, 0, &v);
}

template <typename T>
void mpz_from_any(mpz_t mpz, const T v) requires(T(-1) > T(0))
{
        import(mpz, v);
}

template <typename T>
void mpz_from_any(mpz_t mpz, const T v) requires(T(-1) < T(0))
{
        if (v >= 0)
        {
                import(mpz, v);
                return;
        }
        import(mpz, -v);
        mpz_neg(mpz, mpz);
}
}

template <typename T>
void mpz_from_any(mpz_class* const mpz, const T v)
{
        static_assert(std::is_integral_v<T>);
        *mpz = v;
}

inline void mpz_from_any(mpz_class* const mpz, const long long v)
{
        mpz_implementation::mpz_from_any(mpz->get_mpz_t(), v);
}

inline void mpz_from_any(mpz_class* const mpz, const unsigned long long v)
{
        mpz_implementation::mpz_from_any(mpz->get_mpz_t(), v);
}

inline void mpz_from_any(mpz_class* const mpz, const __int128 v)
{
        mpz_implementation::mpz_from_any(mpz->get_mpz_t(), v);
}

inline void mpz_from_any(mpz_class* const mpz, const unsigned __int128 v)
{
        mpz_implementation::mpz_from_any(mpz->get_mpz_t(), v);
}
}
