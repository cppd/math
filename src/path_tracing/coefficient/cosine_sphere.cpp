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

#include "cosine_sphere.h"

namespace
{
constexpr bool compare(long double v1, long double v2)
{
        long double k = (v1 - v2) / v2;
        k = (k >= 0) ? k : -k;
        return is_finite(v1) && is_finite(v2) && v1 > 0 && v2 > 0 && k < 20 * limits<long double>::epsilon();
}
}

static_assert(compare(cosine_sphere_coefficient(2), PI<long double> / 2));
static_assert(compare(cosine_sphere_coefficient(3), 2));
static_assert(compare(cosine_sphere_coefficient(4), 3 * PI<long double> / 4));
static_assert(compare(cosine_sphere_coefficient(5), 8.0l / 3));
static_assert(compare(cosine_sphere_coefficient(6), 15 * PI<long double> / 16));
static_assert(compare(cosine_sphere_coefficient(7), 16.0l / 5));
static_assert(compare(cosine_sphere_coefficient(8), 35 * PI<long double> / 32));
static_assert(compare(cosine_sphere_coefficient(9), 128.0l / 35));
static_assert(compare(cosine_sphere_coefficient(10), 315 * PI<long double> / 256));
static_assert(compare(cosine_sphere_coefficient(100), 12.501848174018745379275573489380728033040074896079l));
static_assert(compare(cosine_sphere_coefficient(1000), 39.623365897903642007708353245685137074363243183299l));
static_assert(compare(cosine_sphere_coefficient(10000), 125.32828048537769879104381707556904854866773242018l));
