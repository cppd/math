/*
Copyright (C) 2017 Topological Manifold

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
#include <numeric>

static_assert(__GNUC__);
static_assert(__cplusplus >= 201703L);
static_assert(__STDC_HOSTED__ == 1);

static_assert(std::numeric_limits<int>::max() > 2e9);
static_assert(sizeof(int) == 4);

static_assert((static_cast<__int128>(1) << 126) > 0);

static_assert(sizeof(std::complex<float>) == 2 * sizeof(float));
static_assert(sizeof(std::complex<double>) == 2 * sizeof(double));
static_assert(alignof(std::complex<float>) == alignof(float));
static_assert(alignof(std::complex<double>) == alignof(double));
