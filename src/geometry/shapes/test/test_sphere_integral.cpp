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

#include "compare.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace ns::geometry::shapes::test
{
namespace
{
template <typename T>
struct Test final
{
        template <unsigned N>
        static constexpr long double PI_POW = power<N>(PI<long double>);

        template <unsigned N>
        static constexpr bool cmp_unit_over_cosine(const T v)
        {
                return SPHERE_UNIT_INTEGRAL_OVER_COSINE_INTEGRAL<N, T> == v;
        }

        template <unsigned N>
        static constexpr bool cmp_unit_over_cosine(const int epsilon_count, const T v)
        {
                return compare(epsilon_count, SPHERE_UNIT_INTEGRAL_OVER_COSINE_INTEGRAL<N, T>, v);
        }

        template <unsigned N>
        static constexpr bool cmp_integrate(const int epsilon_count, const T v)
        {
                return compare(epsilon_count, SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, T>, v);
        }

        static_assert(cmp_unit_over_cosine<2>(PI<long double> / 2));
        static_assert(cmp_unit_over_cosine<3>(2.0L));
        static_assert(cmp_unit_over_cosine<4>(3 * PI<long double> / 4));
        static_assert(cmp_unit_over_cosine<5>(8.0L / 3));
        static_assert(cmp_unit_over_cosine<6>(15 * PI<long double> / 16));
        static_assert(cmp_unit_over_cosine<7>(16.0L / 5));
        static_assert(cmp_unit_over_cosine<8>(35 * PI<long double> / 32));
        static_assert(cmp_unit_over_cosine<9>(128.0L / 35));
        static_assert(cmp_unit_over_cosine<10>(315 * PI<long double> / 256));
        static_assert(cmp_unit_over_cosine<15>(2048.0L / 429));
        static_assert(cmp_unit_over_cosine<20>(230945 * PI<long double> / 131072));
        static_assert(cmp_unit_over_cosine<25>(4194304.0L / 676039));
        static_assert(cmp_unit_over_cosine<30>(145422675 * PI<long double> / 67108864));
        static_assert(cmp_unit_over_cosine<35>(4294967296.0L / 583401555));
        static_assert(cmp_unit_over_cosine<40>(172308161025 * PI<long double> / 68719476736));
        static_assert(cmp_unit_over_cosine<45>(2199023255552.0L / 263012370465));
        static_assert(cmp_unit_over_cosine<50>(395033145117975 * PI<long double> / 140737488355328));

        static_assert(cmp_unit_over_cosine<100>(5, 12.501848174018745379275573489380728033040074896079L));
        static_assert(cmp_unit_over_cosine<111>(5, 13.174777832962239058614925399585148625028896951069L));
        static_assert(cmp_unit_over_cosine<1000>(10, 39.623365897903642007708353245685137074363243183299L));
        static_assert(cmp_unit_over_cosine<1111>(10, 41.765649734171325590236939525014997796257742486580L));
        static_assert(cmp_unit_over_cosine<10000>(20, 125.32828048537769879104381707556904854866773242018L));
        static_assert(cmp_unit_over_cosine<11111>(20, 132.10727688710841589303636622242392351328925358716L));
        static_assert(cmp_unit_over_cosine<100000>(100, 396.33173893001525509395803345305504249366537658804L));
        static_assert(cmp_unit_over_cosine<111111>(100, 417.77023023440949387785892293393789130459621662998L));

        static_assert(cmp_integrate<2>(1, 2.0L));
        static_assert(cmp_integrate<3>(1, PI_POW<1>));
        static_assert(cmp_integrate<4>(1, 4 * PI_POW<1> / 3));
        static_assert(cmp_integrate<5>(1, PI_POW<2> / 2));
        static_assert(cmp_integrate<6>(2, 8 * PI_POW<2> / 15));
        static_assert(cmp_integrate<7>(1, PI_POW<3> / 6));
        static_assert(cmp_integrate<8>(1, 16 * PI_POW<3> / 105));
        static_assert(cmp_integrate<9>(2, PI_POW<4> / 24));
        static_assert(cmp_integrate<10>(1, 32 * PI_POW<4> / 945));
        static_assert(cmp_integrate<15>(1, PI_POW<7> / 5040));
        static_assert(cmp_integrate<20>(4, 1024 * PI_POW<9> / 654729075));
        static_assert(cmp_integrate<25>(2, PI_POW<12> / 479001600));
        static_assert(cmp_integrate<30>(2, 32768 * PI_POW<14> / 6190283353629375));
        static_assert(cmp_integrate<35>(2, PI_POW<17> / 355687428096000));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

//

template <typename T>
void test_integrate_cosine(const T& precision)
{
        const std::string name = std::string("Test integrate cosine <") + type_name<T>() + ">";

        LOG(name);

        const auto cmp = [&]<std::size_t N>(std::in_place_index_t<N>, const T v1, const T v2, const T v3)
        {
                compare("Test integrate cosine", sphere_integrate_cosine_factor<N, T>(v1, v2), v3, precision);
        };

        // sphereArea[n_] := 2*Power[\[Pi], n/2]/Gamma[n/2];
        // cosineIntegral[n_, a_, b_] :=
        //   sphereArea[n - 1]*
        //    Assuming[n >= 2, Integrate[(Sin[x]^(n - 2))*Cos[x], {x, a, b}]];
        // toString[x_] := If[x == 0, "0.0",
        //    ToString[NumberForm[N[x, 20], {20, 19},
        //      NumberFormat -> (Row[{#1, "e", #3}] &),
        //      ExponentFunction -> (# &)]]];
        // For[n = 2, n <= 10, ++n,
        //  For[i = 0, i <= 3, ++i,
        //   For[j = i + 1, j <= 3, ++j, a = (i/3)*Pi/2; b = (j/3)*Pi/2;
        //    v = cosineIntegral[n, a, b];
        //    Print[StringTemplate[
        //       "cmp(std::in_place_index<``>, ``L, ``L, ``L);"][n, toString[a],
        //      toString[b], toString[v]]]]]]

        cmp(std::in_place_index<2>, 0.0L, 5.2359877559829887308e-1L, 1.0000000000000000000e0L);
        cmp(std::in_place_index<2>, 0.0L, 1.0471975511965977462e0L, 1.7320508075688772935e0L);
        cmp(std::in_place_index<2>, 0.0L, 1.5707963267948966192e0L, 2.0000000000000000000e0L);
        cmp(std::in_place_index<2>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 7.3205080756887729353e-1L);
        cmp(std::in_place_index<2>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 1.0000000000000000000e0L);
        cmp(std::in_place_index<2>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.6794919243112270647e-1L);
        cmp(std::in_place_index<3>, 0.0L, 5.2359877559829887308e-1L, 7.8539816339744830962e-1L);
        cmp(std::in_place_index<3>, 0.0L, 1.0471975511965977462e0L, 2.3561944901923449288e0L);
        cmp(std::in_place_index<3>, 0.0L, 1.5707963267948966192e0L, 3.1415926535897932385e0L);
        cmp(std::in_place_index<3>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 1.5707963267948966192e0L);
        cmp(std::in_place_index<3>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 2.3561944901923449288e0L);
        cmp(std::in_place_index<3>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 7.8539816339744830962e-1L);
        cmp(std::in_place_index<4>, 0.0L, 5.2359877559829887308e-1L, 5.2359877559829887308e-1L);
        cmp(std::in_place_index<4>, 0.0L, 1.0471975511965977462e0L, 2.7206990463513267759e0L);
        cmp(std::in_place_index<4>, 0.0L, 1.5707963267948966192e0L, 4.1887902047863909846e0L);
        cmp(std::in_place_index<4>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 2.1971002707530279028e0L);
        cmp(std::in_place_index<4>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 3.6651914291880921115e0L);
        cmp(std::in_place_index<4>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 1.4680911584350642087e0L);
        cmp(std::in_place_index<5>, 0.0L, 5.2359877559829887308e-1L, 3.0842513753404245684e-1L);
        cmp(std::in_place_index<5>, 0.0L, 1.0471975511965977462e0L, 2.7758262378063821115e0L);
        cmp(std::in_place_index<5>, 0.0L, 1.5707963267948966192e0L, 4.9348022005446793094e0L);
        cmp(std::in_place_index<5>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 2.4674011002723396547e0L);
        cmp(std::in_place_index<5>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 4.6263770630106368526e0L);
        cmp(std::in_place_index<5>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.1589759627382971979e0L);
        cmp(std::in_place_index<6>, 0.0L, 5.2359877559829887308e-1L, 1.6449340668482264365e-1L);
        cmp(std::in_place_index<6>, 0.0L, 1.0471975511965977462e0L, 2.5641984409938253672e0L);
        cmp(std::in_place_index<6>, 0.0L, 1.5707963267948966192e0L, 5.2637890139143245967e0L);
        cmp(std::in_place_index<6>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 2.3997050343090027236e0L);
        cmp(std::in_place_index<6>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 5.0992956072295019531e0L);
        cmp(std::in_place_index<6>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.6995905729204992295e0L);
        cmp(std::in_place_index<7>, 0.0L, 5.2359877559829887308e-1L, 8.0745512188280781707e-2L);
        cmp(std::in_place_index<7>, 0.0L, 1.0471975511965977462e0L, 2.1801288290835811061e0L);
        cmp(std::in_place_index<7>, 0.0L, 1.5707963267948966192e0L, 5.1677127800499700292e0L);
        cmp(std::in_place_index<7>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 2.0993833168953003244e0L);
        cmp(std::in_place_index<7>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 5.0869672678616892475e0L);
        cmp(std::in_place_index<7>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.9875839509663889232e0L);
        cmp(std::in_place_index<8>, 0.0L, 5.2359877559829887308e-1L, 3.6912234143214071637e-2L);
        cmp(std::in_place_index<8>, 0.0L, 1.0471975511965977462e0L, 1.7262143538369862917e0L);
        cmp(std::in_place_index<8>, 0.0L, 1.5707963267948966192e0L, 4.7247659703314011696e0L);
        cmp(std::in_place_index<8>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 1.6893021196937722201e0L);
        cmp(std::in_place_index<8>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 4.6878537361881870980e0L);
        cmp(std::in_place_index<8>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.9985516164944148779e0L);
        cmp(std::in_place_index<9>, 0.0L, 5.2359877559829887308e-1L, 1.5854344243815500852e-2L);
        cmp(std::in_place_index<9>, 0.0L, 1.0471975511965977462e0L, 1.2842018837490555690e0L);
        cmp(std::in_place_index<9>, 0.0L, 1.5707963267948966192e0L, 4.0587121264167682182e0L);
        cmp(std::in_place_index<9>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 1.2683475395052400682e0L);
        cmp(std::in_place_index<9>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 4.0428577821729527173e0L);
        cmp(std::in_place_index<9>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.7745102426677126491e0L);
        cmp(std::in_place_index<10>, 0.0L, 5.2359877559829887308e-1L, 6.4424002006615368543e-3L);
        cmp(std::in_place_index<10>, 0.0L, 1.0471975511965977462e0L, 9.0384372208925467461e-1L);
        cmp(std::in_place_index<10>, 0.0L, 1.5707963267948966192e0L, 3.2985089027387068694e0L);
        cmp(std::in_place_index<10>, 5.2359877559829887308e-1L, 1.0471975511965977462e0L, 8.9740132188859313776e-1L);
        cmp(std::in_place_index<10>, 5.2359877559829887308e-1L, 1.5707963267948966192e0L, 3.2920665025380453325e0L);
        cmp(std::in_place_index<10>, 1.0471975511965977462e0L, 1.5707963267948966192e0L, 2.3946651806494521948e0L);
}

template <typename T>
void test_integrate_power_cosine(const T& precision)
{
        const std::string name = std::string("Test integrate power cosine <") + type_name<T>() + ">";

        LOG(name);

        const auto cmp = [&]<std::size_t N>(std::in_place_index_t<N>, const T v1, const T v2)
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

        LOG(name + " passed");
}

//

template <unsigned N, typename T>
void compare_with_gamma(const T& precision)
{
        static constexpr T CONSTANT = SPHERE_UNIT_INTEGRAL_OVER_COSINE_INTEGRAL<N, T>;

        // sqrt(π) * gamma((k+n)/2) / (gamma((1+k)/2) * gamma(n/2))
        // sqrt(π) * gamma((n+1)/2) / gamma(n/2)
        const T gamma = std::sqrt(PI<T>)
                        * std::exp(std::lgamma(static_cast<T>(N + 1) / 2) - std::lgamma(static_cast<T>(N) / 2));

        const T relative_error = std::abs(gamma - CONSTANT) / std::max(std::abs(gamma), std::abs(CONSTANT));

        if (!(relative_error <= precision))
        {
                std::ostringstream oss;
                oss << std::scientific;
                oss << std::setprecision(Limits<T>::max_digits10());
                oss << "N = " << N << ", gamma = " << gamma << ", constant = " << CONSTANT;
                oss << ", relative error = " << relative_error;
                error("Sphere integral error: " + oss.str());
        }
}

template <typename T>
void compare_with_gamma(const T& precision)
{
        const std::string name = std::string("Compare with gamma <") + type_name<T>() + ">";

        LOG(name);

        [&]<unsigned... I>(std::integer_sequence<unsigned, I...>&&)
        {
                (compare_with_gamma<I + 2, T>(precision), ...);
        }(std::make_integer_sequence<unsigned, 100>());

        compare_with_gamma<1'000, T>(precision);
        compare_with_gamma<1'111, T>(precision);

        LOG(name + " passed");
}

//

template <unsigned N, typename T, bool BY_ANGLE, typename F>
T compute_cosine_weighted_average(const F& f)
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int SAMPLE_COUNT = 100'000;

        PCG engine;
        long double sum = 0;
        long double sum_cosine = 0;
        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
                numerical::Vector<N, T> v;
                T length_square;
                sampling::uniform_in_sphere(engine, v, length_square);

                // dot(v.normalized(), (0, ..., 0, 1))
                const T cosine = std::abs(v[N - 1] / std::sqrt(length_square));

                sum += cosine * f(BY_ANGLE ? std::acos(cosine) : cosine);
                sum_cosine += cosine;
        }

        return sum / sum_cosine;
}

template <unsigned N, typename T>
void check_cosine_weighted_average(const std::string_view description, const T computed, const T average)
{
        constexpr T PRECISION = 1e-2;

        const T relative_error = std::abs(computed - average) / std::max(std::abs(computed), std::abs(average));

        if (relative_error <= PRECISION)
        {
                return;
        }

        std::ostringstream oss;
        oss << "Cosine-weighted average (" << description << ") error: ";
        oss << std::scientific;
        oss << std::setprecision(Limits<T>::max_digits10());
        oss << "N = " << N << ", computed = " << computed << ", average = " << average;
        oss << ", relative error = " << relative_error;
        error(oss.str());
}

template <unsigned N, typename T>
void test_cosine_weighted_average()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int COUNT = 1'000;

        const auto f = [](const T v)
        {
                return 2 * v * v;
        };

        {
                constexpr bool BY_ANGLE = true;
                const T computed = compute_cosine_weighted_average<N, T, BY_ANGLE>(f);
                const T average = sphere_cosine_weighted_average_by_angle<N, T>(f, COUNT);
                check_cosine_weighted_average<N, T>("angle", computed, average);
        }

        {
                constexpr bool BY_ANGLE = false;
                const T computed = compute_cosine_weighted_average<N, T, BY_ANGLE>(f);
                const T average = sphere_cosine_weighted_average_by_cosine<N, T>(f, COUNT);
                check_cosine_weighted_average<N, T>("cosine", computed, average);
        }
}

template <typename T>
void test_cosine_weighted_average()
{
        const std::string name = std::string("Cosine-weighted average <") + type_name<T>() + ">";

        LOG(name);

        test_cosine_weighted_average<2, T>();
        test_cosine_weighted_average<3, T>();
        test_cosine_weighted_average<4, T>();
        test_cosine_weighted_average<5, T>();

        LOG(name + " passed");
}

//

template <unsigned N, typename T>
void test_cosine()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int SAMPLE_COUNT = 100'000;
        constexpr T PRECISION = 1e-2;

        PCG engine;

        long double sum = 0;
        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
                numerical::Vector<N, T> v;
                T length_square;
                sampling::uniform_in_sphere(engine, v, length_square);

                // dot(v.normalized(), (0, ..., 0, 1))
                const T cosine = v[N - 1] / std::sqrt(length_square);

                sum += std::abs(cosine);
        }

        static constexpr long double CONSTANT = SPHERE_UNIT_INTEGRAL_OVER_COSINE_INTEGRAL<N, long double>;
        const long double computed = static_cast<long double>(SAMPLE_COUNT) / sum;
        const long double relative_error =
                std::abs(computed - CONSTANT) / std::max(std::abs(computed), std::abs(CONSTANT));

        if (!(relative_error <= PRECISION))
        {
                std::ostringstream oss;
                oss << std::fixed;
                oss << std::setprecision(Limits<long double>::max_digits10());
                oss << "N = " << std::setw(2) << N << ", computed = " << computed << ", constant = " << CONSTANT;
                oss << ", relative error = ";
                oss << std::setprecision(7) << relative_error;
                error("Sphere integral error: " + oss.str());
        }
}

template <typename T>
void test_cosine()
{
        const std::string name = std::string("Test cosine sphere <") + type_name<T>() + ">";

        LOG(name);

        test_cosine<2, T>();
        test_cosine<3, T>();
        test_cosine<4, T>();
        test_cosine<5, T>();

        LOG(name + " passed");
}

//

void test_sphere_integral()
{
        test_integrate_cosine<float>(6e-7);
        test_integrate_cosine<double>(2e-15);
        test_integrate_cosine<long double>(7e-19);

        test_integrate_power_cosine<float>(1e-3);
        test_integrate_power_cosine<double>(1e-12);
        test_integrate_power_cosine<long double>(1e-15);

        compare_with_gamma<float>(1e-3);
        compare_with_gamma<double>(1e-12);
        compare_with_gamma<long double>(1e-15);

        test_cosine_weighted_average<float>();
        test_cosine_weighted_average<double>();

        test_cosine<float>();
        test_cosine<double>();
}

TEST_SMALL("Sphere Integral", test_sphere_integral)
}
}
