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

#include "compare.h"

#include "../sphere_integral.h"

#include <src/com/constant.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <cmath>
#include <random>
#include <sstream>
#include <version>

namespace ns::geometry
{
namespace
{
using shapes::test::compare;

template <unsigned N>
constexpr long double PI_POW = power<N>(PI<long double>);

static_assert(sphere_unit_integral_over_cosine_integral(2) == PI<long double> / 2);
static_assert(sphere_unit_integral_over_cosine_integral(3) == 2.0L);
static_assert(sphere_unit_integral_over_cosine_integral(4) == 3 * PI<long double> / 4);
static_assert(sphere_unit_integral_over_cosine_integral(5) == 8.0L / 3);
static_assert(sphere_unit_integral_over_cosine_integral(6) == 15 * PI<long double> / 16);
static_assert(sphere_unit_integral_over_cosine_integral(7) == 16.0L / 5);
static_assert(sphere_unit_integral_over_cosine_integral(8) == 35 * PI<long double> / 32);
static_assert(sphere_unit_integral_over_cosine_integral(9) == 128.0L / 35);
static_assert(sphere_unit_integral_over_cosine_integral(10) == 315 * PI<long double> / 256);
static_assert(sphere_unit_integral_over_cosine_integral(15) == 2048.0L / 429);
static_assert(sphere_unit_integral_over_cosine_integral(20) == 230945 * PI<long double> / 131072);
static_assert(sphere_unit_integral_over_cosine_integral(25) == 4194304.0L / 676039);
static_assert(sphere_unit_integral_over_cosine_integral(30) == 145422675 * PI<long double> / 67108864);
static_assert(sphere_unit_integral_over_cosine_integral(35) == 4294967296.0L / 583401555);
static_assert(sphere_unit_integral_over_cosine_integral(40) == 172308161025 * PI<long double> / 68719476736);
static_assert(sphere_unit_integral_over_cosine_integral(45) == 2199023255552.0L / 263012370465);
static_assert(sphere_unit_integral_over_cosine_integral(50) == 395033145117975 * PI<long double> / 140737488355328);

static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(100),
                12.501848174018745379275573489380728033040074896079L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(111),
                13.174777832962239058614925399585148625028896951069L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(1000),
                39.623365897903642007708353245685137074363243183299L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(1111),
                41.765649734171325590236939525014997796257742486580L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(10000),
                125.32828048537769879104381707556904854866773242018L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(11111),
                132.10727688710841589303636622242392351328925358716L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(100000),
                396.33173893001525509395803345305504249366537658804L));
static_assert(
        compare(100,
                sphere_unit_integral_over_cosine_integral(111111),
                417.77023023440949387785892293393789130459621662998L));

static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<2>(), 2.0L));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<3>(), PI_POW<1>));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<4>(), 4 * PI_POW<1> / 3));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<5>(), PI_POW<2> / 2));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<6>(), 8 * PI_POW<2> / 15));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<7>(), PI_POW<3> / 6));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<8>(), 16 * PI_POW<3> / 105));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<9>(), PI_POW<4> / 24));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<10>(), 32 * PI_POW<4> / 945));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<15>(), PI_POW<7> / 5040));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<20>(), 1024 * PI_POW<9> / 654729075));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<25>(), PI_POW<12> / 479001600));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<30>(), 32768 * PI_POW<14> / 6190283353629375));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere<35>(), PI_POW<17> / 355687428096000));

template <typename T>
void test_integrate_power_cosine(T precision)
{
        LOG(std::string("Test integrate power cosine, ") + type_name<T>());

        const auto cmp = [&]<std::size_t N>(std::in_place_index_t<N>, T v1, T v2)
        {
                compare("Test integrate power cosine", sphere_integrate_power_cosine_factor_over_hemisphere<N, T>(v1),
                        v2, precision);
        };

        // hemisphereArea[n_]:=Power[\[Pi],n/2]/Gamma[n/2];
        // unitIntegral[n_]:=Integrate[Sin[x]^(n-2),{x,0,Pi/2}];
        // cosineIntegral[n_,k_]:=Integrate[(Sin[x]^(n-2))*(Cos[x]^k),{x,0,Pi/2}];
        // func[n_,k_]:=hemisphereArea[n]*(cosineIntegral[n,k]/unitIntegral[n]);
        // For[n=2,n<=10,++n,For[k=0,k<=3,++k,v=func[n,10^k];
        //   Print[StringTemplate["cmp(std::in_place_index<``>, 1e``, ``L);"][n,k,N[v, 50]]]]]

        cmp(std::in_place_index<2>, 1e0, 2.0L);
        cmp(std::in_place_index<2>, 1e1, 0.77312631709436317977791614510394016290789715687747L);
        cmp(std::in_place_index<2>, 1e2, 0.25003696348037490758551146978761456066080149792158L);
        cmp(std::in_place_index<2>, 1e3, 0.079246731795807284015416706491370274148726486366598L);
        cmp(std::in_place_index<3>, 1e0, 3.1415926535897932384626433832795028841971693993751L);
        cmp(std::in_place_index<3>, 1e1, 0.57119866428905331608411697877809143349039443625002L);
        cmp(std::in_place_index<3>, 1e2, 0.062209755516629569078468185807514908597963750482675L);
        cmp(std::in_place_index<3>, 1e3, 0.0062769083987808056712540327338251805878065322664837L);
        cmp(std::in_place_index<4>, 1e0, 4.1887902047863909846168578443726705122628925325001L);
        cmp(std::in_place_index<4>, 1e1, 0.40480799301343072460063341991679526140935095225011L);
        cmp(std::in_place_index<4>, 1e2, 0.015402240933251867250640251287456962737692424806669L);
        cmp(std::in_place_index<4>, 1e3, 0.00049692804477187394461107220124159712495436671990011L);
        cmp(std::in_place_index<5>, 1e0, 4.9348022005446793094172454999380755676568497036204L);
        cmp(std::in_place_index<5>, 1e1, 0.27607285038012191940795779020632590588290068272002L);
        cmp(std::in_place_index<5>, 1e2, 0.0037949070080128265380503666249643953226237429230956L);
        cmp(std::in_place_index<5>, 1e3, 0.000039321015578994718616715252842376571127033283395531L);
        cmp(std::in_place_index<6>, 1e0, 5.2637890139143245967117285332672806055006396838618L);
        cmp(std::in_place_index<6>, 1e1, 0.18167740242363175884068153359629723751303880019073L);
        cmp(std::in_place_index<6>, 1e2, 0.00093053013393700129062975439425952977673536370279759L);
        cmp(std::in_place_index<6>, 1e3, 0.0000031098515833029064666159090575156652922335669169073L);
        cmp(std::in_place_index<7>, 1e0, 5.1677127800499700292460525111835658670375480943142L);
        cmp(std::in_place_index<7>, 1e1, 0.11564112514797135729781376248802385157007100630633L);
        cmp(std::in_place_index<7>, 1e2, 0.00022708670433199082810710328113850964384614539627038L);
        cmp(std::in_place_index<7>, 1e3, 0.00000024583206701424799651751305250139528444532289338027L);
        cmp(std::in_place_index<8>, 1e0, 4.7247659703314011695963908673678316498629011148015L);
        cmp(std::in_place_index<8>, 1e1, 0.071344549097169753835283446793485188903609071537611L);
        cmp(std::in_place_index<8>, 1e2, 0.000055157483636234143408976140317340552215126256404544L);
        cmp(std::in_place_index<8>, 1e3, 0.000000019423234369500989792288528560159602123737218994805L);
        cmp(std::in_place_index<9>, 1e0, 4.0587121264167682181850138620293796354053160696952L);
        cmp(std::in_place_index<9>, 1e1, 0.042740859907967612663733589003682484025658614308596L);
        cmp(std::in_place_index<9>, 1e2, 0.000013334839664622427312232434090139618843436747588113L);
        cmp(std::in_place_index<9>, 1e3, 0.0000000015338713321723043557133813083875444267582961447094L);
        cmp(std::in_place_index<10>, 1e0, 3.2985089027387068693821065037445117036944790915618L);
        cmp(std::in_place_index<10>, 1e1, 0.024903945701927201600157984215774382037784888234707L);
        cmp(std::in_place_index<10>, 1e2, 0.0000032089323218906003781602335385785508268283638231843L);
        cmp(std::in_place_index<10>, 1e3, 0.00000000012107121111939898632517568543157447062530555860635L);

        LOG("Check passed");
}

//

#if defined(__cpp_lib_math_special_functions) && __cpp_lib_math_special_functions >= 201603L
template <typename T>
T beta(std::type_identity_t<T> x, std::type_identity_t<T> y)
{
        static_assert(std::is_floating_point_v<T>);
        return std::beta(x, y);
}
#else
#if !defined(__clang__)
#error __cpp_lib_math_special_functions
#endif
template <typename T>
T beta(std::type_identity_t<T> x, std::type_identity_t<T> y)
{
        static_assert(std::is_floating_point_v<T>);
        // Β(x, y) = Γ(x) * Γ(y) / Γ(x + y)
        return std::exp(std::lgamma(x) + std::lgamma(y) - std::lgamma(x + y));
}
#endif

void compare_with_beta(unsigned n)
{
        long double v_beta = beta<long double>(0.5L, (n - 1) / 2.0L) / beta<long double>(1.0L, (n - 1) / 2.0L);
        long double v_function = sphere_unit_integral_over_cosine_integral(n);
        long double discrepancy_percent = std::abs((v_beta - v_function) / v_function) * 100;

        if (!(discrepancy_percent <= 1e-10))
        {
                std::ostringstream oss;
                oss << std::fixed;
                oss << std::setprecision(limits<long double>::max_digits10);
                oss << "N = " << n << ", beta = " << v_beta << ", function = " << v_function;
                oss << std::scientific;
                oss << std::setprecision(5);
                oss << ", discrepancy = " << discrepancy_percent << "%";

                error("Huge discrepancy between beta and function: " + oss.str());
        }
}

void compare_with_beta()
{
        LOG("Compare with beta");

        for (unsigned n = 2; n < 10'000; ++n)
        {
                compare_with_beta(n);
        }
        for (unsigned n = 10'000; n <= 1'000'000; (n & 1) == 0 ? ++n : n += 999)
        {
                compare_with_beta(n);
        }

        LOG("Check passed");
}

//

template <std::size_t N, typename T>
void test_cosine()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int COUNT = 10'000'000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        long double sum = 0;
        for (int i = 0; i < COUNT; ++i)
        {
                Vector<N, T> v;
                T length_square;
                sampling::uniform_in_sphere(engine, v, length_square);
                // dot(v.normalized(), (0, ..., 0, 1))
                T c = v[N - 1] / std::sqrt(length_square);
                sum += std::abs(c);
        }

        long double data = static_cast<long double>(COUNT) / sum;
        long double function = sphere_unit_integral_over_cosine_integral(N);
        long double discrepancy_percent = std::abs((data - function) / function) * 100;

        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(limits<long double>::max_digits10);
        oss << "N = " << std::setw(2) << N << ", data = " << data << ", function = " << function;
        oss << std::setprecision(5);
        oss << ", discrepancy = " << discrepancy_percent << "%";
        LOG(oss.str());

        if (!(discrepancy_percent <= 0.1))
        {
                error("Huge discrepancy between data and function: " + oss.str());
        }
}

template <typename T, std::size_t... I>
void test_cosine(std::integer_sequence<std::size_t, I...>)
{
        (test_cosine<I + 2, T>(), ...);
}

template <typename T>
void test_cosine()
{
        LOG(std::string("Test cosine sphere, ") + type_name<T>());

        test_cosine<T>(std::make_integer_sequence<std::size_t, 10>());

        LOG("Check passed");
}

void test_sphere_integral_small()
{
        test_integrate_power_cosine<float>(1e-3);
        test_integrate_power_cosine<double>(1e-12);
        test_integrate_power_cosine<long double>(1e-16);
}

void test_sphere_integral_large()
{
        compare_with_beta();
        LOG("");
        test_cosine<float>();
        LOG("");
        test_cosine<double>();
}

TEST_SMALL("Sphere Integral", test_sphere_integral_small)
TEST_LARGE("Sphere Integral", test_sphere_integral_large)
}
}
