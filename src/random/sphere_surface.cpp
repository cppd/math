/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "sphere_surface.h"

#include <src/com/math.h>
#include <src/com/type/limit.h>

namespace ns::random
{
namespace
{
template <typename T>
constexpr bool compare(int epsilon_count, T v1, T v2)
{
        static_assert(std::is_floating_point_v<T>);
        return is_finite(v1) && is_finite(v2) && (v1 > 0) && (v2 > 0)
               && v2 > (v1 - v1 * (epsilon_count * limits<T>::epsilon()))
               && v2 < (v1 + v1 * (epsilon_count * limits<T>::epsilon()))
               && v1 > (v2 - v2 * (epsilon_count * limits<T>::epsilon()))
               && v1 < (v2 + v2 * (epsilon_count * limits<T>::epsilon()));
}

template <unsigned N>
constexpr long double pi_pow = power<N>(PI<long double>);
}

static_assert(compare(1, 1.1, 1.1));
static_assert(compare(1000, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000002, 10000.100000001));

static_assert(cosine_sphere_coefficient(2) == PI<long double> / 2);
static_assert(cosine_sphere_coefficient(3) == 2.0L);
static_assert(cosine_sphere_coefficient(4) == 3 * PI<long double> / 4);
static_assert(cosine_sphere_coefficient(5) == 8.0L / 3);
static_assert(cosine_sphere_coefficient(6) == 15 * PI<long double> / 16);
static_assert(cosine_sphere_coefficient(7) == 16.0L / 5);
static_assert(cosine_sphere_coefficient(8) == 35 * PI<long double> / 32);
static_assert(cosine_sphere_coefficient(9) == 128.0L / 35);
static_assert(cosine_sphere_coefficient(10) == 315 * PI<long double> / 256);
static_assert(cosine_sphere_coefficient(15) == 2048.0L / 429);
static_assert(cosine_sphere_coefficient(20) == 230945 * PI<long double> / 131072);
static_assert(cosine_sphere_coefficient(25) == 4194304.0L / 676039);
static_assert(cosine_sphere_coefficient(30) == 145422675 * PI<long double> / 67108864);
static_assert(cosine_sphere_coefficient(35) == 4294967296.0L / 583401555);
static_assert(cosine_sphere_coefficient(40) == 172308161025 * PI<long double> / 68719476736);
static_assert(cosine_sphere_coefficient(45) == 2199023255552.0L / 263012370465);
static_assert(cosine_sphere_coefficient(50) == 395033145117975 * PI<long double> / 140737488355328);

static_assert(compare(100, cosine_sphere_coefficient(100), 12.501848174018745379275573489380728033040074896079L));
static_assert(compare(100, cosine_sphere_coefficient(111), 13.174777832962239058614925399585148625028896951069L));
static_assert(compare(100, cosine_sphere_coefficient(1000), 39.623365897903642007708353245685137074363243183299L));
static_assert(compare(100, cosine_sphere_coefficient(1111), 41.765649734171325590236939525014997796257742486580L));
static_assert(compare(100, cosine_sphere_coefficient(10000), 125.32828048537769879104381707556904854866773242018L));
static_assert(compare(100, cosine_sphere_coefficient(11111), 132.10727688710841589303636622242392351328925358716L));
static_assert(compare(100, cosine_sphere_coefficient(100000), 396.33173893001525509395803345305504249366537658804L));
static_assert(compare(100, cosine_sphere_coefficient(111111), 417.77023023440949387785892293393789130459621662998L));

static_assert(compare(10, sphere_area(2), 2 * pi_pow<1>));
static_assert(compare(10, sphere_area(3), 4 * pi_pow<1>));
static_assert(compare(10, sphere_area(4), 2 * pi_pow<2>));
static_assert(compare(10, sphere_area(5), 8 * pi_pow<2> / 3));
static_assert(compare(10, sphere_area(6), pi_pow<3>));
static_assert(compare(10, sphere_area(7), 16 * pi_pow<3> / 15));
static_assert(compare(10, sphere_area(8), pi_pow<4> / 3));
static_assert(compare(10, sphere_area(9), 32 * pi_pow<4> / 105));
static_assert(compare(10, sphere_area(10), pi_pow<5> / 12));
static_assert(compare(10, sphere_area(15), 256 * pi_pow<7> / 135135));
static_assert(compare(10, sphere_area(20), pi_pow<10> / 181440));
static_assert(compare(10, sphere_area(25), 8192 * pi_pow<12> / 316234143225));
static_assert(compare(10, sphere_area(30), pi_pow<15> / 43589145600));
static_assert(compare(10, sphere_area(35), 262144 * pi_pow<17> / 6332659870762850625));
static_assert(compare(10, sphere_area(40), pi_pow<20> / 60822550204416000));

static_assert(compare(10, sphere_area(45), 1.2876986762598652169610927230442052274087372377085e-9L));
static_assert(compare(10, sphere_area(50), 8.6510962291805538057726365290958840196659212205551e-12L));
static_assert(compare(10, sphere_area(100), 2.3682021018828339613111743245754170110390710827884e-38L));
static_assert(compare(10, sphere_area(111), 4.5744152213753183840687985785233817617533382664144e-45L));
}
