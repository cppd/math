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

#include "com/type/limit.h"

namespace
{
template <typename T>
constexpr bool compare(int epsilon_count, T v1, T v2)
{
        static_assert(std::is_floating_point_v<T>);

        T k = (v1 - v2) / v2;
        k = (k >= 0) ? k : -k;
        return is_finite(v1) && is_finite(v2) && (v1 > 0) && (v2 > 0) && (k < epsilon_count * limits<T>::epsilon());
}
}

static_assert(cosine_sphere_coefficient(2) == PI<long double> / 2);
static_assert(cosine_sphere_coefficient(3) == 2.0l);
static_assert(cosine_sphere_coefficient(4) == 3 * PI<long double> / 4);
static_assert(cosine_sphere_coefficient(5) == 8.0l / 3);
static_assert(cosine_sphere_coefficient(6) == 15 * PI<long double> / 16);
static_assert(cosine_sphere_coefficient(7) == 16.0l / 5);
static_assert(cosine_sphere_coefficient(8) == 35 * PI<long double> / 32);
static_assert(cosine_sphere_coefficient(9) == 128.0l / 35);
static_assert(cosine_sphere_coefficient(10) == 315 * PI<long double> / 256);
static_assert(cosine_sphere_coefficient(15) == 2048.0l / 429);
static_assert(cosine_sphere_coefficient(20) == 230945 * PI<long double> / 131072);
static_assert(cosine_sphere_coefficient(25) == 4194304.0l / 676039);
static_assert(cosine_sphere_coefficient(30) == 145422675 * PI<long double> / 67108864);
static_assert(cosine_sphere_coefficient(35) == 4294967296.0l / 583401555);
static_assert(cosine_sphere_coefficient(40) == 172308161025 * PI<long double> / 68719476736);
static_assert(cosine_sphere_coefficient(45) == 2199023255552.0l / 263012370465);
static_assert(cosine_sphere_coefficient(50) == 395033145117975 * PI<long double> / 140737488355328);

static_assert(compare(100, cosine_sphere_coefficient(100), 12.501848174018745379275573489380728033040074896079l));
static_assert(compare(100, cosine_sphere_coefficient(111), 13.174777832962239058614925399585148625028896951069l));
static_assert(compare(100, cosine_sphere_coefficient(1000), 39.623365897903642007708353245685137074363243183299l));
static_assert(compare(100, cosine_sphere_coefficient(1111), 41.765649734171325590236939525014997796257742486580l));
static_assert(compare(100, cosine_sphere_coefficient(10000), 125.32828048537769879104381707556904854866773242018l));
static_assert(compare(100, cosine_sphere_coefficient(11111), 132.10727688710841589303636622242392351328925358716l));
static_assert(compare(100, cosine_sphere_coefficient(100000), 396.33173893001525509395803345305504249366537658804l));
static_assert(compare(100, cosine_sphere_coefficient(111111), 417.77023023440949387785892293393789130459621662998l));
