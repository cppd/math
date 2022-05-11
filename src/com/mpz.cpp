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

#include "mpz.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

namespace ns
{
namespace
{
template <typename T>
T to_int(const mpz_t mpz)
{
        if (mpz_sizeinbase(mpz, Limits<T>::radix()) > static_cast<std::size_t>(Limits<T>::digits()))
        {
                error("mpz size " + to_string(mpz_sizeinbase(mpz, Limits<T>::radix())) + " is too large for "
                      + to_string(Limits<T>::digits()) + " digit integer");
        }
        T res = 0;
        mpz_export(&res, nullptr, -1, sizeof(res), 0, 0, mpz);
        return res;
}

template <typename T>
T mpz_to_int(const mpz_t mpz) requires(T(-1) > T(0))
{
        return to_int<T>(mpz);
}

template <typename T>
T mpz_to_int(const mpz_t mpz) requires(T(-1) < T(0))
{
        if (mpz_sgn(mpz) >= 0)
        {
                return to_int<T>(mpz);
        }
        return -to_int<T>(mpz);
}

template <typename T>
T mpz_to_int(const mpz_class& mpz)
{
        return mpz_to_int<T>(mpz.get_mpz_t());
}

template <typename T>
void compare(const T v)
{
        const mpz_class mpz = [&]
        {
                mpz_class res;
                mpz_from_any(&res, v);
                return res;
        }();

        const T mpz_value = mpz_to_int<T>(mpz);
        if (mpz_value == v)
        {
                return;
        }

        error("Error importing mpz, integer value " + to_string_digit_groups(v) + " is not equal to mpz value "
              + to_string_digit_groups(mpz_value));
}

void test()
{
        compare(static_cast<std::int_least8_t>(0x70));
        compare(-static_cast<std::int_least8_t>(0x70));
        compare(static_cast<std::uint_least8_t>(0xF0));

        compare(static_cast<std::int_least16_t>(0x7000));
        compare(-static_cast<std::int_least16_t>(0x7000));
        compare(static_cast<std::uint_least16_t>(0xF000));

        compare(static_cast<std::int_least32_t>(0x7000'FFFF));
        compare(-static_cast<std::int_least32_t>(0x7000'FFFF));
        compare(static_cast<std::uint_least32_t>(0xF000'FFFF));

        compare(static_cast<long long>(0x7000'FFFF'FFFF'FFFF));
        compare(-static_cast<long long>(0x7000'FFFF'FFFF'FFFF));
        compare(static_cast<unsigned long long>(0xF000'FFFF'FFFF'FFFF));

        compare(static_cast<__int128>(0x7000'FFFF'FFFF'FFFF) << 64);
        compare(-(static_cast<__int128>(0x7000'FFFF'FFFF'FFFF) << 64));
        compare(static_cast<unsigned __int128>(0xF000'FFFF'FFFF'FFFF) << 64);
}

TEST_SMALL("GMP integer import", test)
}
}
