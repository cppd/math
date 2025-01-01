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

#include <src/com/type/limit.h>

#include <limits>

namespace ns
{
namespace type_limit_implementation
{
namespace
{
static_assert(binary_epsilon<float>() == std::numeric_limits<float>::epsilon());
static_assert(binary_epsilon<double>() == std::numeric_limits<double>::epsilon());
static_assert(binary_epsilon<long double>() == std::numeric_limits<long double>::epsilon());

static_assert((1 + binary_epsilon<float>() > 1) && (1 + binary_epsilon<float>() / 2 == 1));
static_assert((1 + binary_epsilon<double>() > 1) && (1 + binary_epsilon<double>() / 2 == 1));
static_assert((1 + binary_epsilon<long double>() > 1) && (1 + binary_epsilon<long double>() / 2 == 1));
static_assert((1 + binary_epsilon<__float128>() > 1) && (1 + binary_epsilon<__float128>() / 2 == 1));

static_assert(2 - binary_epsilon<float>() == max_binary_fraction<float>());
static_assert(2 - binary_epsilon<double>() == max_binary_fraction<double>());
static_assert(2 - binary_epsilon<long double>() == max_binary_fraction<long double>());
static_assert(2 - binary_epsilon<__float128>() == max_binary_fraction<__float128>());

static_assert(1 == binary_exponent<float>(0));
static_assert(1 == binary_exponent<double>(0));

static_assert(std::numeric_limits<float>::max() == max_binary_fraction<float>() * binary_exponent<float>(127));
static_assert(std::numeric_limits<float>::min() == binary_exponent<float>(-126));
static_assert(std::numeric_limits<double>::max() == max_binary_fraction<double>() * binary_exponent<double>(1023));
static_assert(std::numeric_limits<double>::min() == binary_exponent<double>(-1022));
}
}

namespace
{
template <typename T>
class CompareWithNumericLimits final
{
        static_assert(Limits<T>::epsilon() == std::numeric_limits<T>::epsilon());
        static_assert(Limits<T>::max() == std::numeric_limits<T>::max());
        static_assert(Limits<T>::lowest() == std::numeric_limits<T>::lowest());
        static_assert(Limits<T>::infinity() == std::numeric_limits<T>::infinity());
        static_assert(Limits<T>::digits() == std::numeric_limits<T>::digits);
        static_assert(Limits<T>::digits10() == std::numeric_limits<T>::digits10);
        static_assert(Limits<T>::max_digits10() == std::numeric_limits<T>::max_digits10);
        static_assert(Limits<T>::radix() == std::numeric_limits<T>::radix);
        static_assert(Limits<T>::is_iec559() == std::numeric_limits<T>::is_iec559);
};
template class CompareWithNumericLimits<float>;
template class CompareWithNumericLimits<double>;
template class CompareWithNumericLimits<long double>;

static_assert(Limits<unsigned __int128>::max() > 0);
static_assert(Limits<unsigned __int128>::max() == (((static_cast<unsigned __int128>(1) << 127) - 1) << 1) + 1);
static_assert(Limits<unsigned __int128>::max() + 1 == 0);
static_assert(Limits<unsigned __int128>::max() == static_cast<unsigned __int128>(-1));
static_assert(Limits<unsigned __int128>::lowest() == 0);

static_assert(Limits<signed __int128>::max() > 0);
static_assert(Limits<signed __int128>::lowest() < 0);
static_assert(static_cast<unsigned __int128>(Limits<signed __int128>::max()) == Limits<unsigned __int128>::max() >> 1);
static_assert(
        (static_cast<unsigned __int128>(1) << 127)
        == static_cast<unsigned __int128>(Limits<signed __int128>::max()) + 1);
static_assert(Limits<signed __int128>::lowest() + 1 + Limits<signed __int128>::max() == 0);
}
}
