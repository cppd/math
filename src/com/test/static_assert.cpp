/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <complex>
#include <limits>
#include <type_traits>

namespace ns
{
static_assert(std::numeric_limits<int>::max() >= (1ull << 31) - 1);
static_assert(std::numeric_limits<unsigned>::max() >= (1ull << 32) - 1);

static_assert(std::is_same_v<__int128, signed __int128>);
static_assert((static_cast<__int128>(1) << 126) > 0);
static_assert((static_cast<unsigned __int128>(1) << 127) > 0);

static_assert(std::numeric_limits<float>::is_iec559);
static_assert(std::numeric_limits<double>::is_iec559);
static_assert(std::numeric_limits<long double>::is_iec559);

static_assert(sizeof(std::complex<float>) == 2 * sizeof(float));
static_assert(sizeof(std::complex<double>) == 2 * sizeof(double));
static_assert(sizeof(std::complex<long double>) == 2 * sizeof(long double));
static_assert(alignof(std::complex<float>) == alignof(float));
static_assert(alignof(std::complex<double>) == alignof(double));
static_assert(alignof(std::complex<long double>) == alignof(long double));

static_assert(u8'\t' == '\t' && u8'\n' == '\n' && u8'\r' == '\r' && u8' ' == ' ');
static_assert(u8'*' == '*' && u8'0' == '0' && u8'9' == '9' && u8'=' == '=');
static_assert(u8'a' == 'a' && u8'z' == 'z' && u8'A' == 'A' && u8'Z' == 'Z');
}
