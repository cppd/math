/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "../sphere_area.h"

#include <src/com/constant.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <string>

namespace ns::geometry::shapes::test
{
namespace
{
template <unsigned N>
constexpr long double PI_POW = power<N>(PI<long double>);

template <typename T>
struct Test final
{
        static_assert(compare<T>(1, SPHERE_AREA<2, T>, 2 * PI_POW<1>));
        static_assert(compare<T>(1, SPHERE_AREA<3, T>, 4 * PI_POW<1>));
        static_assert(compare<T>(1, SPHERE_AREA<4, T>, 2 * PI_POW<2>));
        static_assert(compare<T>(1, SPHERE_AREA<5, T>, 8 * PI_POW<2> / 3));
        static_assert(compare<T>(1, SPHERE_AREA<6, T>, PI_POW<3>));
        static_assert(compare<T>(1, SPHERE_AREA<7, T>, 16 * PI_POW<3> / 15));
        static_assert(compare<T>(2, SPHERE_AREA<8, T>, PI_POW<4> / 3));
        static_assert(compare<T>(1, SPHERE_AREA<9, T>, 32 * PI_POW<4> / 105));
        static_assert(compare<T>(1, SPHERE_AREA<10, T>, PI_POW<5> / 12));
        static_assert(compare<T>(3, SPHERE_AREA<15, T>, 256 * PI_POW<7> / 135135));
        static_assert(compare<T>(1, SPHERE_AREA<20, T>, PI_POW<10> / 181440));
        static_assert(compare<T>(3, SPHERE_AREA<25, T>, 8192 * PI_POW<12> / 316234143225));
        static_assert(compare<T>(3, SPHERE_AREA<30, T>, PI_POW<15> / 43589145600));
        static_assert(compare<T>(5, SPHERE_AREA<35, T>, 262144 * PI_POW<17> / 6332659870762850625));
        static_assert(compare<T>(5, SPHERE_AREA<40, T>, PI_POW<20> / 60822550204416000));

        static_assert(compare<T>(5, SPHERE_AREA<45, T>, 1.2876986762598652169610927230442052274087372377085e-9L));
        static_assert(compare<T>(3, SPHERE_AREA<50, T>, 8.6510962291805538057726365290958840196659212205551e-12L));
        static_assert(compare<T>(7, SPHERE_AREA<100, T>, 2.3682021018828339613111743245754170110390710827884e-38L));

        static_assert(
                (std::is_same_v<T, float>)
                || compare<T>(8, SPHERE_AREA<111, T>, 4.5744152213753183840687985785233817617533382664144e-45L));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

template <typename T>
void test_sphere_relative_area_1(const T precision)
{
        LOG(std::string("Test sphere area 1, ") + type_name<T>());

        const auto cmp = [&](const T v1, const T v2)
        {
                compare("Test sphere relative area 1", v1, v2, precision);
        };

        // For[i=2,i<=15,++i,s=N[Integrate[Sin[x]^(i-2),{x,0,2/100}],50];Print[s]]

        constexpr T A = 0;
        constexpr T B = T{2} / 100;

        cmp(sphere_relative_area<2, T>(A, B), 0.020000000000000000000000000000000000000000000000000L);
        cmp(sphere_relative_area<3, T>(A, B), 0.00019999333342222158730440916252473687391190040787159L);
        cmp(sphere_relative_area<4, T>(A, B), 2.6664533414601368632970710213651858296386621067898e-6L);
        cmp(sphere_relative_area<5, T>(A, B), 3.9994667013319450114137130142823942202673515800668e-8L);
        cmp(sphere_relative_area<6, T>(A, B), 6.3987810661520318292459940262167674422671211810225e-10L);
        cmp(sphere_relative_area<7, T>(A, B), 1.0664000327085489960575319571348081646746327644674e-11L);
        cmp(sphere_relative_area<8, T>(A, B), 1.8280026265588533512161872871460315952717790721539e-13L);
        cmp(sphere_relative_area<9, T>(A, B), 3.1988055523296056052387212070528515453783560176867e-15L);
        cmp(sphere_relative_area<10, T>(A, B), 5.6864069967024130174728000244489525713993372131728e-17L);
        cmp(sphere_relative_area<11, T>(A, B), 1.0234881257856185906451937695326598229087769952864e-18L);
        cmp(sphere_relative_area<12, T>(A, B), 1.8607682166264188457074516587007738024188554947031e-20L);
        cmp(sphere_relative_area<13, T>(A, B), 3.4111884727144566012904172410298843679358511573332e-22L);
        cmp(sphere_relative_area<14, T>(A, B), 6.2971708851622969919328243435987148096048176384326e-24L);
        cmp(sphere_relative_area<15, T>(A, B), 1.1693985788590365916906420253926137382086446806823e-25L);

        LOG("Check passed");
}

template <typename T>
void test_sphere_relative_area_2(const T precision)
{
        LOG(std::string("Test sphere area 2, ") + type_name<T>());

        const auto cmp = [&](const T v1, const T v2)
        {
                compare("Test sphere relative area 2", v1, v2, precision);
        };

        // For[i=2,i<=15,++i,s=N[Integrate[Sin[x]^(i-2),{x,1/2,1}],50];Print[s]]

        constexpr T A = 0.5;
        constexpr T B = 1;

        cmp(sphere_relative_area<2, T>(A, B), 0.50000000000000000000000000000000000000000000000000L);
        cmp(sphere_relative_area<3, T>(A, B), 0.33728025602223299871534497516085304825933477649182L);
        cmp(sphere_relative_area<4, T>(A, B), 0.23304338949555370281412061392963853923007702233762L);
        cmp(sphere_relative_area<5, T>(A, B), 0.16456605049432905175652851085684561857127023868729L);
        cmp(sphere_relative_area<6, T>(A, B), 0.11847776692887839197760002141640185370388427675061L);
        cmp(sphere_relative_area<7, T>(A, B), 0.086747410598336502855863559308529083473508300192666L);
        cmp(sphere_relative_area<8, T>(A, B), 0.064445032897166510836125417254910295152840007397306L);
        cmp(sphere_relative_area<9, T>(A, B), 0.048475825004558812194932172261776921435799662926282L);
        cmp(sphere_relative_area<10, T>(A, B), 0.036852689606665752354152799788873530801949717378474L);
        cmp(sphere_relative_area<11, T>(A, B), 0.028271142654439652603483734391164058265792744319845L);
        cmp(sphere_relative_area<12, T>(A, B), 0.021856353187699151682891120312318245519917593143986L);
        cmp(sphere_relative_area<13, T>(A, B), 0.017009720583937844245155790468162021432350290550126L);
        cmp(sphere_relative_area<14, T>(A, B), 0.013313970393473262087067334544828366956211559294135L);
        cmp(sphere_relative_area<15, T>(A, B), 0.010473262061717212781929422559521292732168015614157L);

        LOG("Check passed");
}

void test_sphere_area()
{
        test_sphere_relative_area_1<double>(0.02);
        test_sphere_relative_area_1<long double>(0.002);

        test_sphere_relative_area_2<float>(2e-4);
        test_sphere_relative_area_2<double>(2e-4);
        test_sphere_relative_area_2<long double>(2e-4);
}

TEST_SMALL("Sphere Area", test_sphere_area)
}
}
