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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/painter/pixels/gaussian.h>
#include <src/test/test.h>

#include <cstddef>

namespace ns::painter::pixels
{
namespace
{
template <typename T>
void compare(const T a, const T b, const T precision)
{
        if (a == b)
        {
                return;
        }
        const T abs = std::abs(a - b);
        const T max = std::max(std::abs(a), std::abs(b));
        if (!(abs / max < precision))
        {
                error("Values are not equal: " + to_string(a) + " and " + to_string(b));
        }
}

template <typename T, std::size_t N>
void compare(const T width, const T radius, const Vector<N, T>& p, const T value, const T precision)
{
        compare(Gaussian<T>(width, radius).compute(p), value, precision);
}

template <typename T>
void test_filter(const T precision)
{
        // gaussian[width_, x_] := Exp[-1/2*Power[ x/width, 2]];
        // filter[width_, radius_, list_] :=
        //   Module[{e, m}, e = gaussian[width, radius]; m = 1;
        //    Do[m *= Max[0, gaussian[width, v] - e], {v, list}]; m];
        //
        // toString[x_] :=
        //   ToString[NumberForm[N[x, 50], {50, 49},
        //     NumberFormat -> (Row[{#1, "e", #3}] &),
        //     ExponentFunction -> (# &)]];
        // filterString[width_, radius_, list_] :=
        //   StringTemplate["cmp(``, ``, Vector<``, T>``, ``L);"][width, radius,
        //    Length[list], list, toString[filter[width, radius, list]]];
        //
        // random[x_] := RandomInteger[{-x*100, x*100}]/100;
        // randomVector2[x_] := {random[x], random[x]};
        // randomVector3[x_] := {random[x], random[x], random[x]};
        //
        // randomRadius[x_] := RandomInteger[{1, x*10}]/10;
        // randomWidth[x_] := x*(1/10 + RandomInteger[{0, 100}]/100 * 9/10);
        //
        // SeedRandom[1234567890];
        // maxRadius = 10;
        // For[i = 1, i <= 10, ++i, radius = randomRadius[maxRadius];
        //  Print[filterString[randomWidth[radius], radius,
        //    randomVector2[radius]]]]
        // For[i = 1, i <= 10, ++i, radius = randomRadius[maxRadius];
        //  Print[filterString[randomWidth[radius], radius,
        //    randomVector3[radius]]]]

        const auto cmp = [&](const auto& width, const auto& radius, const auto& p, const auto& value)
        {
                compare<T>(width, radius, p, value, precision);
        };

        cmp(4.2336, 5.4, Vector<2, T>{2.62, -2.73}, 1.4109221134291896820342830287343161724303362324826e-1L);
        cmp(0.2524, 0.4, Vector<2, T>{0.15, -0.21}, 2.3379348961259863315287249105519171829868249168293e-1L);
        cmp(2.2378, 6.7, Vector<2, T>{6.38, 1.12}, 5.1104758764574898056340532792369226760916162322032e-3L);
        cmp(3.7393, 6.1, Vector<2, T>{3.06, 4.72}, 8.4143796239328175045622047829851847854310801034958e-2L);
        cmp(6.2169, 6.9, Vector<2, T>{2.15, -5.06}, 7.1480079203568063960255534396488552264297264198913e-2L);
        cmp(3.3704, 4.4, Vector<2, T>{4.37, -0.83}, 2.7006586273475784520618247465626495299152258349655e-3L);
        cmp(1.558, 1.9, Vector<2, T>{-1.61, 1.23}, 2.8484255727495089508296566105085365093691901719328e-2L);
        cmp(1.7259, 3.3, Vector<2, T>{0.28, 2.06}, 2.7244715347823298674767598393175022176288271807782e-1L);
        cmp(2.5992, 7.2, Vector<2, T>{-6.95, -2.04}, 4.6039631370139695204471391441041224354312984500489e-3L);
        cmp(0.6685, 0.7, Vector<2, T>{-0.21, 0.31}, 1.1967566060895380104320789478054579446602584608813e-1L);
        cmp(5.236, 7, Vector<3, T>{-6.25, 4.11, -3.14}, 1.1287239465162204408169713648359617443705269123834e-2L);
        cmp(7.1378, 8.9, Vector<3, T>{-3.25, 6.22, -1.25}, 5.2091886409702059553363608284959984318588866113450e-2L);
        cmp(1.584, 4.5, Vector<3, T>{1.28, -0.9, -4.16}, 8.2745820734033906461645411845649017286155956761299e-3L);
        cmp(0.8921, 1.1, Vector<3, T>{0.18, 0.52, 0.47}, 7.7633699271733648216308181423711931252561928190066e-2L);
        cmp(2.1625, 2.5, Vector<3, T>{1.67, 2.36, -1.72}, 1.9199769643770466668971638260458171799174994905234e-3L);
        cmp(4.646, 9.2, Vector<3, T>{4.89, -7.89, 4.82}, 1.8394606196437268057063077683091834676745831143480e-2L);
        cmp(2.2056, 2.4, Vector<3, T>{-0.91, -1.3, 1.33}, 2.9440413734632932640781610310838711948789230630593e-2L);
        cmp(4.2434, 9.8, Vector<3, T>{3.95, -9.35, 7.9}, 1.1665033645438299011574674737890744720108866591882e-3L);
        cmp(3.4237, 7.3, Vector<3, T>{-1.83, -4.74, -6.83}, 7.2274156464984008576119347961069146647970056259574e-3L);
        cmp(4.1622, 4.2, Vector<3, T>{-3.7, -2.84, -4.17}, 6.0694821644412464793250481641248184564817380910464e-5L);
}

void test()
{
        test_filter<float>(1e-5);
        test_filter<double>(1e-13);
}

TEST_SMALL("Painter Filter", test)
}
}
