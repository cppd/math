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

#include <limits>
#include <type_traits>

namespace ns
{
namespace types_implementation
{
template <typename T>
constexpr T binary_epsilon()
{
        T prev_e = 1;
        T e = 1;
        T s = 1;
        do
        {
                prev_e = e;
                e /= 2;
                s = 1 + e;
        } while (s != 1);
        return prev_e;
}

template <typename T>
constexpr T max_binary_fraction()
{
        // max_binary_fraction = 2 - binary_epsilon
        T prev_r = 1;
        T r = 1;
        T a = 1;
        do
        {
                prev_r = r;
                a /= 2;
                r += a;
        } while (r != 2);
        return prev_r;
}

template <typename T>
constexpr T binary_exponent(int e)
{
        if (e == 0)
        {
                return 1;
        }
        T r = 1;
        if (e > 0)
        {
                for (int i = 1; i <= e; ++i)
                {
                        r *= 2;
                }
        }
        if (e < 0)
        {
                e = -e;
                for (int i = 1; i <= e; ++i)
                {
                        r /= 2;
                }
        }
        return r;
}

template <typename T>
constexpr bool INTEGRAL_SPECIALIZED = std::is_integral_v<T>&& std::numeric_limits<T>::is_specialized;

template <typename T>
constexpr bool FLOATING_POINT_SPECIALIZED = std::is_floating_point_v<T>&& std::numeric_limits<T>::is_specialized;

template <typename T, typename = void>
class limits;

template <typename T>
class limits<T, std::enable_if_t<INTEGRAL_SPECIALIZED<T>>>
{
        static_assert(INTEGRAL_SPECIALIZED<T>);

public:
        static constexpr T max()
        {
                return std::numeric_limits<T>::max();
        }
        static constexpr T lowest()
        {
                return std::numeric_limits<T>::lowest();
        }
        static constexpr int digits = std::numeric_limits<T>::digits;
        static constexpr int digits10 = std::numeric_limits<T>::digits10;
        static constexpr int radix = std::numeric_limits<T>::radix;
};

template <typename T>
class limits<T, std::enable_if_t<FLOATING_POINT_SPECIALIZED<T>>>
{
        static_assert(FLOATING_POINT_SPECIALIZED<T>);

public:
        static constexpr T epsilon()
        {
                return std::numeric_limits<T>::epsilon();
        }
        static constexpr T max()
        {
                return std::numeric_limits<T>::max();
        }
        static constexpr T lowest()
        {
                return std::numeric_limits<T>::lowest();
        }
        static constexpr int digits = std::numeric_limits<T>::digits;
        static constexpr int digits10 = std::numeric_limits<T>::digits10;
        static constexpr int max_digits10 = std::numeric_limits<T>::max_digits10;
        static constexpr int radix = std::numeric_limits<T>::radix;
};

template <typename T>
class limits<T, std::enable_if_t<std::is_same_v<T, unsigned __int128> && !INTEGRAL_SPECIALIZED<T>>>
{
        static_assert(std::is_same_v<T, unsigned __int128>);
        static_assert(!INTEGRAL_SPECIALIZED<T>);

public:
        static constexpr T max()
        {
                return -1;
        }
        static constexpr T lowest()
        {
                return 0;
        }
        static constexpr int digits = 128;
        static constexpr int digits10 = 38;
        static constexpr int radix = 2;
};

template <typename T>
class limits<T, std::enable_if_t<std::is_same_v<T, signed __int128> && !INTEGRAL_SPECIALIZED<T>>>
{
        static_assert(std::is_same_v<T, signed __int128>);
        static_assert(!INTEGRAL_SPECIALIZED<T>);

public:
        static constexpr T max()
        {
                return static_cast<unsigned __int128>(-1) >> 1;
        }
        static constexpr T lowest()
        {
                return -max() - 1;
        }
        static constexpr int digits = 127;
        static constexpr int digits10 = 38;
        static constexpr int radix = 2;
};

template <typename T>
class limits<T, std::enable_if_t<std::is_same_v<T, __float128> && !FLOATING_POINT_SPECIALIZED<T>>>
{
        static_assert(std::is_same_v<T, __float128>);
        static_assert(!FLOATING_POINT_SPECIALIZED<T>);

        // epsilon = strtoflt128("1.92592994438723585305597794258492732e-34", nullptr)
        // max = strtoflt128("1.18973149535723176508575932662800702e4932", nullptr)

public:
        static constexpr T epsilon()
        {
                return binary_epsilon<T>();
        }
        static constexpr T max()
        {
                return max_binary_fraction<T>() * binary_exponent<T>(16383);
        }
        static constexpr T lowest()
        {
                return -max();
        }
        static constexpr int digits = 113;
        static constexpr int digits10 = 33;
        static constexpr int max_digits10 = 36;
        static constexpr int radix = 2;
};
}

//

template <typename T>
class limits : public types_implementation::limits<T>
{
};
template <typename T>
class limits<const T> : public types_implementation::limits<T>
{
};
template <typename T>
class limits<volatile T> : public types_implementation::limits<T>
{
};
template <typename T>
class limits<const volatile T> : public types_implementation::limits<T>
{
};
}
