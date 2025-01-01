/*
Copyright (C) 2017-2025 Topological Manifold

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
namespace type_limit_implementation
{
template <typename T>
constexpr T binary_epsilon() noexcept
{
        T prev_e = 1;
        T e = 1;
        do
        {
                prev_e = e;
                e /= 2;
        } while (1 + e != 1);
        return prev_e;
}

template <typename T>
constexpr T max_binary_fraction() noexcept
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
constexpr T binary_exponent(const int e) noexcept
{
        if (e > 0)
        {
                T res = 1;
                for (int i = 1; i <= e; ++i)
                {
                        res *= 2;
                }
                return res;
        }

        if (e < 0)
        {
                T res = 1;
                for (int i = -1; i >= e; --i)
                {
                        res /= 2;
                }
                return res;
        }

        return 1;
}

template <typename T>
concept IntegralType =
        std::is_integral_v<T> || std::is_same_v<T, signed __int128> || std::is_same_v<T, unsigned __int128>;

template <typename T>
concept FloatingPointType = std::is_floating_point_v<T> || std::is_same_v<T, __float128>;

template <typename T>
struct Limits;

template <typename T>
        requires (IntegralType<T> && std::numeric_limits<T>::is_specialized)
struct Limits<T>
{
        static constexpr T max() noexcept
        {
                return std::numeric_limits<T>::max();
        }

        static constexpr T lowest() noexcept
        {
                return std::numeric_limits<T>::lowest();
        }

        static constexpr int digits() noexcept
        {
                return std::numeric_limits<T>::digits;
        }

        static constexpr int digits10() noexcept
        {
                return std::numeric_limits<T>::digits10;
        }

        static constexpr int radix() noexcept
        {
                return std::numeric_limits<T>::radix;
        }
};

template <typename T>
        requires (FloatingPointType<T> && std::numeric_limits<T>::is_specialized)
struct Limits<T>
{
        static constexpr T epsilon() noexcept
        {
                return std::numeric_limits<T>::epsilon();
        }

        static constexpr T max() noexcept
        {
                return std::numeric_limits<T>::max();
        }

        static constexpr T lowest() noexcept
        {
                return std::numeric_limits<T>::lowest();
        }

        static constexpr T infinity() noexcept
        {
                static_assert(std::numeric_limits<T>::has_infinity);
                return std::numeric_limits<T>::infinity();
        }

        static constexpr int digits() noexcept
        {
                return std::numeric_limits<T>::digits;
        }

        static constexpr int digits10() noexcept
        {
                return std::numeric_limits<T>::digits10;
        }

        static constexpr int max_digits10() noexcept
        {
                return std::numeric_limits<T>::max_digits10;
        }

        static constexpr int max_exponent() noexcept
        {
                return std::numeric_limits<T>::max_exponent;
        }

        static constexpr int radix() noexcept
        {
                return std::numeric_limits<T>::radix;
        }

        static constexpr bool is_iec559() noexcept
        {
                return std::numeric_limits<T>::is_iec559;
        }
};

template <typename T>
        requires (std::is_same_v<T, unsigned __int128> && !std::numeric_limits<T>::is_specialized)
struct Limits<T>
{
        static constexpr T max() noexcept
        {
                return -1;
        }

        static constexpr T lowest() noexcept
        {
                return 0;
        }

        static constexpr int digits() noexcept
        {
                return 128;
        }

        static constexpr int digits10() noexcept
        {
                return 38;
        }

        static constexpr int radix() noexcept
        {
                return 2;
        }
};

template <typename T>
        requires (std::is_same_v<T, signed __int128> && !std::numeric_limits<T>::is_specialized)
struct Limits<T>
{
        static constexpr T max() noexcept
        {
                return static_cast<unsigned __int128>(-1) >> 1;
        }

        static constexpr T lowest() noexcept
        {
                return -max() - 1;
        }

        static constexpr int digits() noexcept
        {
                return 127;
        }

        static constexpr int digits10() noexcept
        {
                return 38;
        }

        static constexpr int radix() noexcept
        {
                return 2;
        }
};

template <typename T>
        requires (std::is_same_v<T, __float128> && !std::numeric_limits<T>::is_specialized)
struct Limits<T>
{
        // epsilon = strtoflt128("1.92592994438723585305597794258492732e-34", nullptr)
        // max = strtoflt128("1.18973149535723176508575932662800702e4932", nullptr)

        static constexpr T epsilon() noexcept
        {
                return binary_epsilon<T>();
        }

        static constexpr T max() noexcept
        {
                return max_binary_fraction<T>() * binary_exponent<T>(16383);
        }

        static constexpr T lowest() noexcept
        {
                return -max();
        }

        static constexpr int digits() noexcept
        {
                return 113;
        }

        static constexpr int digits10() noexcept
        {
                return 33;
        }

        static constexpr int max_digits10() noexcept
        {
                return 36;
        }

        static constexpr int max_exponent() noexcept
        {
                return 16384;
        }

        static constexpr int radix() noexcept
        {
                return 2;
        }

        static constexpr bool is_iec559() noexcept
        {
                return true;
        }
};
}

template <typename T>
struct Limits final : type_limit_implementation::Limits<T>
{
};

template <typename T>
struct Limits<const T> final : type_limit_implementation::Limits<T>
{
};

template <typename T>
struct Limits<volatile T> final : type_limit_implementation::Limits<T>
{
};

template <typename T>
struct Limits<const volatile T> final : type_limit_implementation::Limits<T>
{
};
}
