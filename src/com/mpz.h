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

#pragma once

#include <gmpxx.h>

#if 0

#include "error.h"

inline unsigned long long mpz_to_ull(mpz_t z)
{
        if (mpz_cmp(z, mpz_class("18446744073709551615").get_mpz_t()) > 0)
        {
                error("error convert mpz to unsigned long long");
        }

        unsigned long long result = 0;
        mpz_export(&result, nullptr, -1, sizeof(result), 0, 0, z);
        return result;
}

#endif

namespace mpz_implementation
{
inline void mpz_from_llong(mpz_t z, unsigned long long v)
{
        mpz_import(z, 1, -1, sizeof(v), 0, 0, &v);
}
inline void mpz_from_llong(mpz_t z, long long v)
{
        if (v >= 0)
        {
                mpz_from_llong(z, static_cast<unsigned long long>(v));
        }
        else
        {
                mpz_from_llong(z, static_cast<unsigned long long>(-v));
                mpz_neg(z, z);
        }
}

inline void mpz_from_int128(mpz_t z, unsigned __int128 v)
{
        mpz_import(z, 1, -1, sizeof(v), 0, 0, &v);
}
inline void mpz_from_int128(mpz_t z, __int128 v)
{
        if (v >= 0)
        {
                mpz_from_int128(z, static_cast<unsigned __int128>(v));
        }
        else
        {
                mpz_from_int128(z, static_cast<unsigned __int128>(-v));
                mpz_neg(z, z);
        }
}
}

template <typename T>
void mpz_from_any(mpz_class* z, T v)
{
        *z = v;
}
inline void mpz_from_any(mpz_class* z, unsigned __int128 v)
{
        mpz_implementation::mpz_from_int128(z->get_mpz_t(), v);
}
inline void mpz_from_any(mpz_class* z, __int128 v)
{
        mpz_implementation::mpz_from_int128(z->get_mpz_t(), v);
}
inline void mpz_from_any(mpz_class* z, unsigned long long v)
{
        mpz_implementation::mpz_from_llong(z->get_mpz_t(), v);
}
inline void mpz_from_any(mpz_class* z, long long v)
{
        mpz_implementation::mpz_from_llong(z->get_mpz_t(), v);
}

#if 0
class MPZ : public mpz_class
{
public:
        MPZ() = default;
        MPZ(const MPZ& v) = default;
        MPZ& operator=(const MPZ& v) = default;
        MPZ(MPZ&& v) = default;
        MPZ& operator=(MPZ&& v) = default;
        ~MPZ() = default;

        template <typename... T>
        MPZ(T&&... v) : mpz_class(std::forward<T>(v)...)
        {
        }
        MPZ(unsigned long long v)
        {
                *this = v;
        }
        MPZ(long long v)
        {
                *this = v;
        }

        template <typename T>
        MPZ& operator=(T&& v)
        {
                *(static_cast<mpz_class*>(this)) = std::forward<T>(v);
                return *this;
        }
        MPZ& operator=(unsigned long long v)
        {
                mpz_from_any(this, v);
                return *this;
        }
        MPZ& operator=(long long v)
        {
                mpz_from_any(this, v);
                return *this;
        }
};
#endif
