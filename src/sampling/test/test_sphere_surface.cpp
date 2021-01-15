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

#include "test_sphere_surface.h"

#include "../sphere_surface.h"
#include "../sphere_uniform.h"

#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <random>
#include <sstream>
#include <version>

namespace ns::sampling
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

static_assert(compare(1, 1.1, 1.1));
static_assert(compare(1000, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000002, 10000.100000001));

template <unsigned N>
constexpr long double pi_pow = power<N>(PI<long double>);

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

static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(2), 2.0L));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(3), pi_pow<1>));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(4), 4 * pi_pow<1> / 3));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(5), pi_pow<2> / 2));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(6), 8 * pi_pow<2> / 15));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(7), pi_pow<3> / 6));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(8), 16 * pi_pow<3> / 105));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(9), pi_pow<4> / 24));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(10), 32 * pi_pow<4> / 945));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(15), pi_pow<7> / 5040));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(20), 1024 * pi_pow<9> / 654729075));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(25), pi_pow<12> / 479001600));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(30), 32768 * pi_pow<14> / 6190283353629375));
static_assert(compare(10, sphere_integrate_cosine_factor_over_hemisphere(35), pi_pow<17> / 355687428096000));

template <typename T>
void compare(T v1, T v2, T precision)
{
        if (!(is_finite(v1) && is_finite(v2) && ((v1 == v2) || std::abs((v1 - v2) / std::max(v1, v2)) < precision)))
        {
                error("Numbers are not equal " + to_string(v1) + " and " + to_string(v2));
        }
}

template <typename T>
void test_sphere_relative_area_1(T precision)
{
        LOG(std::string("Test sphere area 1, ") + type_name<T>());

        const auto cmp = [&](T v1, T v2)
        {
                compare(v1, v2, precision);
        };

        // For[i=2,i<=15,++i,s=N[Integrate[Sin[x]^(i-2),{x,0,2/100}],50];Print[s]]

        constexpr T a = 0;
        constexpr T b = T(2) / 100;
        cmp(sphere_relative_area<2, T>(a, b), T(0.020000000000000000000000000000000000000000000000000L));
        cmp(sphere_relative_area<3, T>(a, b), T(0.00019999333342222158730440916252473687391190040787159L));
        cmp(sphere_relative_area<4, T>(a, b), T(2.6664533414601368632970710213651858296386621067898e-6L));
        cmp(sphere_relative_area<5, T>(a, b), T(3.9994667013319450114137130142823942202673515800668e-8L));
        cmp(sphere_relative_area<6, T>(a, b), T(6.3987810661520318292459940262167674422671211810225e-10L));
        cmp(sphere_relative_area<7, T>(a, b), T(1.0664000327085489960575319571348081646746327644674e-11L));
        cmp(sphere_relative_area<8, T>(a, b), T(1.8280026265588533512161872871460315952717790721539e-13L));
        cmp(sphere_relative_area<9, T>(a, b), T(3.1988055523296056052387212070528515453783560176867e-15L));
        cmp(sphere_relative_area<10, T>(a, b), T(5.6864069967024130174728000244489525713993372131728e-17L));
        cmp(sphere_relative_area<11, T>(a, b), T(1.0234881257856185906451937695326598229087769952864e-18L));
        cmp(sphere_relative_area<12, T>(a, b), T(1.8607682166264188457074516587007738024188554947031e-20L));
        cmp(sphere_relative_area<13, T>(a, b), T(3.4111884727144566012904172410298843679358511573332e-22L));
        cmp(sphere_relative_area<14, T>(a, b), T(6.2971708851622969919328243435987148096048176384326e-24L));
        cmp(sphere_relative_area<15, T>(a, b), T(1.1693985788590365916906420253926137382086446806823e-25L));

        LOG("Check passed");
}

template <typename T>
void test_sphere_relative_area_2(T precision)
{
        LOG(std::string("Test sphere area 2, ") + type_name<T>());

        const auto cmp = [&](T v1, T v2)
        {
                compare(v1, v2, precision);
        };

        // For[i=2,i<=15,++i,s=N[Integrate[Sin[x]^(i-2),{x,1/2,1}],50];Print[s]]

        constexpr T a = 0.5;
        constexpr T b = 1;
        cmp(sphere_relative_area<2, T>(a, b), T(0.50000000000000000000000000000000000000000000000000L));
        cmp(sphere_relative_area<3, T>(a, b), T(0.33728025602223299871534497516085304825933477649182L));
        cmp(sphere_relative_area<4, T>(a, b), T(0.23304338949555370281412061392963853923007702233762L));
        cmp(sphere_relative_area<5, T>(a, b), T(0.16456605049432905175652851085684561857127023868729L));
        cmp(sphere_relative_area<6, T>(a, b), T(0.11847776692887839197760002141640185370388427675061L));
        cmp(sphere_relative_area<7, T>(a, b), T(0.086747410598336502855863559308529083473508300192666L));
        cmp(sphere_relative_area<8, T>(a, b), T(0.064445032897166510836125417254910295152840007397306L));
        cmp(sphere_relative_area<9, T>(a, b), T(0.048475825004558812194932172261776921435799662926282L));
        cmp(sphere_relative_area<10, T>(a, b), T(0.036852689606665752354152799788873530801949717378474L));
        cmp(sphere_relative_area<11, T>(a, b), T(0.028271142654439652603483734391164058265792744319845L));
        cmp(sphere_relative_area<12, T>(a, b), T(0.021856353187699151682891120312318245519917593143986L));
        cmp(sphere_relative_area<13, T>(a, b), T(0.017009720583937844245155790468162021432350290550126L));
        cmp(sphere_relative_area<14, T>(a, b), T(0.013313970393473262087067334544828366956211559294135L));
        cmp(sphere_relative_area<15, T>(a, b), T(0.010473262061717212781929422559521292732168015614157L));

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

        constexpr int count = 10'000'000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        long double sum = 0;
        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v;
                T length_square;
                uniform_in_sphere(engine, v, length_square);
                // косинус угла между вектором и последней координатной осью
                T c = v[N - 1] / std::sqrt(length_square);
                sum += std::abs(c);
        }

        long double data = static_cast<long double>(count) / sum;
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
}

void test_sphere_surface(bool all_tests)
{
        test_sphere_relative_area_1<double>(0.02);
        test_sphere_relative_area_1<long double>(0.002);

        test_sphere_relative_area_2<float>(2e-4);
        test_sphere_relative_area_2<double>(2e-4);
        test_sphere_relative_area_2<long double>(2e-4);

        if (!all_tests)
        {
                return;
        }

        LOG("");
        compare_with_beta();
        LOG("");
        test_cosine<float>();
        LOG("");
        test_cosine<double>();
}
}
