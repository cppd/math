/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <gmp.h>
#include <gmpxx.h>
#include <type_traits>

namespace ns
{
namespace set_mpz_implementation
{
template <typename T>
void import_mpz(mpz_t mpz, const T v)
{
        static_assert(
                (std::is_integral_v<T>) || (std::is_same_v<std::remove_cv_t<T>, unsigned __int128>)
                || (std::is_same_v<std::remove_cv_t<T>, signed __int128>));

        mpz_import(mpz, 1, -1, sizeof(v), 0, 0, &v);
}

template <typename T>
        requires (static_cast<T>(-1) > T{0})
void set_mpz(mpz_t mpz, const T v)
{
        import_mpz(mpz, v);
}

template <typename T>
        requires (static_cast<T>(-1) < T{0})
void set_mpz(mpz_t mpz, const T v)
{
        if (v >= 0)
        {
                import_mpz(mpz, v);
                return;
        }
        import_mpz(mpz, -v);
        mpz_neg(mpz, mpz);
}
}

template <typename T>
void set_mpz(mpz_class* const mpz, const T v)
{
        static_assert(std::is_integral_v<T>);
        *mpz = v;
}

inline void set_mpz(mpz_class* const mpz, const long long v)
{
        set_mpz_implementation::set_mpz(mpz->get_mpz_t(), v);
}

inline void set_mpz(mpz_class* const mpz, const unsigned long long v)
{
        set_mpz_implementation::set_mpz(mpz->get_mpz_t(), v);
}

inline void set_mpz(mpz_class* const mpz, const __int128 v)
{
        set_mpz_implementation::set_mpz(mpz->get_mpz_t(), v);
}

inline void set_mpz(mpz_class* const mpz, const unsigned __int128 v)
{
        set_mpz_implementation::set_mpz(mpz->get_mpz_t(), v);
}
}
