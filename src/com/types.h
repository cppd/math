/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include <limits>
#include <string>
#include <type_traits>

namespace types_implementation
{
template <typename T>
constexpr T binary_epsilon() noexcept
{
        T prev_e = 1, e = 1, s = 1;
        do
        {
                prev_e = e;
                e /= 2;
                s = 1 + e;
        } while (s != 1);
        return prev_e;
}

template <typename T>
constexpr T max_binary_fraction() noexcept
{
        // max_binary_fraction = 2 - binary_epsilon
        T prev_r = 1, r = 1, a = 1;
        do
        {
                prev_r = r;
                a /= 2;
                r += a;
        } while (r != 2);
        return prev_r;
}

template <typename T>
constexpr T binary_exponent(int e) noexcept
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

template <typename T, bool = true>
class limits
{
        static_assert(std::numeric_limits<T>::is_specialized);
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

public:
        static constexpr T epsilon() noexcept
        {
                static_assert(std::is_floating_point_v<T>);
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
        static constexpr int digits = std::numeric_limits<T>::digits;
        static constexpr int digits10 = std::numeric_limits<T>::digits10;
        static constexpr int radix = std::numeric_limits<T>::radix;
};

template <>
class limits<unsigned __int128, !std::numeric_limits<unsigned __int128>::is_specialized>
{
        using T = unsigned __int128;

public:
        static constexpr T max() noexcept
        {
                return -1;
        }
        static constexpr T lowest() noexcept
        {
                return 0;
        }
        static constexpr int digits = 128;
        static constexpr int digits10 = 38;
        static constexpr int radix = 2;
};

template <>
class limits<signed __int128, !std::numeric_limits<signed __int128>::is_specialized>
{
        using T = signed __int128;

public:
        static constexpr T max() noexcept
        {
                return static_cast<unsigned __int128>(-1) >> 1;
        }
        static constexpr T lowest() noexcept
        {
                return -max() - 1;
        }
        static constexpr int digits = 127;
        static constexpr int digits10 = 38;
        static constexpr int radix = 2;
};

template <>
class limits<__float128, !std::numeric_limits<__float128>::is_specialized>
{
        using T = __float128;

        // epsilon = strtoflt128("1.92592994438723585305597794258492732e-34", nullptr)
        // max = strtoflt128("1.18973149535723176508575932662800702e4932", nullptr)

public:
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
        static constexpr int digits = 113;
        static constexpr int digits10 = 33;
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

template <typename T>
inline constexpr bool is_native_integral = std::is_same_v<std::remove_cv_t<T>, unsigned __int128> ||
                                           std::is_same_v<std::remove_cv_t<T>, signed __int128> || std::is_integral_v<T>;
template <typename T>
inline constexpr bool is_integral = is_native_integral<T> || std::is_same_v<std::remove_cv_t<T>, mpz_class>;

template <typename T>
inline constexpr bool is_native_floating_point = std::is_same_v<std::remove_cv_t<T>, __float128> || std::is_floating_point_v<T>;
template <typename T>
inline constexpr bool is_floating_point = is_native_floating_point<T> || std::is_same_v<std::remove_cv_t<T>, mpf_class>;

template <typename T>
inline constexpr bool is_signed =
        std::is_same_v<std::remove_cv_t<T>, mpz_class> || std::is_same_v<std::remove_cv_t<T>, mpf_class> ||
        std::is_same_v<std::remove_cv_t<T>, __int128> || std::is_same_v<std::remove_cv_t<T>, __float128> || std::is_signed_v<T>;

template <typename T>
inline constexpr bool is_unsigned = std::is_same_v<std::remove_cv_t<T>, unsigned __int128> || std::is_unsigned_v<T>;

// clang-format off
template<int BITS>
using LeastSignedInteger =
        std::conditional_t<BITS <=   7, int_least8_t,
        std::conditional_t<BITS <=  15, int_least16_t,
        std::conditional_t<BITS <=  31, int_least32_t,
        std::conditional_t<BITS <=  63, int_least64_t,
        std::conditional_t<BITS <= 127, signed __int128,
        mpz_class>>>>>;

template<int BITS>
using LeastUnsignedInteger =
        std::conditional_t<BITS <=   8, uint_least8_t,
        std::conditional_t<BITS <=  16, uint_least16_t,
        std::conditional_t<BITS <=  32, uint_least32_t,
        std::conditional_t<BITS <=  64, uint_least64_t,
        std::conditional_t<BITS <= 128, unsigned __int128,
        mpz_class>>>>>;
// clang-format on

//

template <typename T>
std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, mpz_class>, std::string> type_str()
{
        return "mpz_class";
}
template <typename T>
std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, mpf_class>, std::string> type_str()
{
        return "mpf_class";
}
template <typename T>
std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, mpq_class>, std::string> type_str()
{
        return "mpq_class";
}
template <typename T>
std::enable_if_t<is_native_integral<T>, std::string> type_str()
{
        return std::to_string(limits<T>::digits) + " bits";
}
template <typename T>
std::enable_if_t<is_native_floating_point<T>, std::string> type_str()
{
        return "fp " + std::to_string(limits<T>::digits) + " bits";
}

//

template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, float>, const char*> type_name()
{
        return "float";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, double>, const char*> type_name()
{
        return "double";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, long double>, const char*> type_name()
{
        return "long double";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, __float128>, const char*> type_name()
{
        return "__float128";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, mpf_class>, const char*> type_name()
{
        return "mpf_class";
}

template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, float>, const char*> floating_point_suffix()
{
        return "f";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, double>, const char*> floating_point_suffix()
{
        return "";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, long double>, const char*> floating_point_suffix()
{
        return "l";
}

// C++20
// std::type_identity и std::type_identity_t
// Может использоваться для запрета class template argument deduction.
template <typename T>
struct TypeIdentity
{
        using type = T;
};
template <class T>
using type_identity_t = typename TypeIdentity<T>::type;
