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

#include "../xyz.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>

namespace ns::color
{
namespace
{
template <typename T>
void check(T a, T b, T precision, T min)
{
        if (a == b)
        {
                return;
        }
        if (a >= 0 && a < min && b >= 0 && b < min)
        {
                return;
        }
        T max = std::max(std::abs(a), std::abs(b));
        if (!(std::abs(a - b) / max < precision))
        {
                LOG("Numbers are not equal: " + to_string(a) + ", " + to_string(b));
        }
}

/*
integral[x1_, x2_, a_, m_, t1_, t2_] :=
  If[x2 <= m, Integrate[a*Exp[-1/2*(t1*(x - m))^2], {x, x1, x2}],
   If[x1 >= m, Integrate[a*Exp[-1/2*(t2*(x - m))^2], {x, x1, x2}],
    Integrate[a*Exp[-1/2*(t1*(x - m))^2], {x, x1, m}] +
     Integrate[a*Exp[-1/2*(t2*(x - m))^2], {x, m, x2}]]];

x[w1_, w2_] :=
  integral[w1, w2, 0.362`30, 442.0`30, 0.0624`30, 0.0374`30] +
   integral[w1, w2, 1.056`30, 599.8`30, 0.0264`30, 0.0323`30] +
   integral[w1, w2, -0.065`30, 501.1`30, 0.0490`30, 0.0382`30];
y[w1_, w2_] :=
  integral[w1, w2, 0.821`30, 568.8`30, 0.0213`30, 0.0247`30] +
   integral[w1, w2, 0.286`30, 530.9`30, 0.0613`30, 0.0322`30];
z[w1_, w2_] :=
  integral[w1, w2, 1.217`30, 437.0`30, 0.0845`30, 0.0278`30] +
   integral[w1, w2, 0.681`30, 459.0`30, 0.0385`30, 0.0725`30];

For[i = 380, i < 780, i += 100,
 For[j = i + 100, j <= 780, j += 100,
  Print[StringTemplate["c(cie_x_integral<XYZ_31, T>(``, ``), ``L);"][
    i, j, x[i, j]]];
  Print[StringTemplate["c(cie_y_integral<XYZ_31, T>(``, ``), ``L);"][
    i, j, y[i, j]]];
  Print[StringTemplate["c(cie_z_integral<XYZ_31, T>(``, ``), ``L);"][
    i, j, z[i, j]]]
  ]]
*/

template <typename T>
void test_31(T precision)
{
        const auto c = [&](T a, T b)
        {
                check<T>(a, b, precision, 1e-9);
        };

        c(cie_x_integral<XYZ_31, T>(380, 480), 17.0952220179951084163111128L);
        c(cie_y_integral<XYZ_31, T>(380, 480), 2.8369922178039151633727727L);
        c(cie_z_integral<XYZ_31, T>(380, 480), 92.5756043882350714446630041L);
        c(cie_x_integral<XYZ_31, T>(380, 580), 45.7495256157044115379327972L);
        c(cie_y_integral<XYZ_31, T>(380, 580), 73.0966753264988021675236244L);
        c(cie_z_integral<XYZ_31, T>(380, 580), 106.8024486322991300664397085L);
        c(cie_x_integral<XYZ_31, T>(380, 680), 106.3209305552937374493450019L);
        c(cie_y_integral<XYZ_31, T>(380, 680), 106.6929555257343980229673484L);
        c(cie_z_integral<XYZ_31, T>(380, 680), 106.8063035964889478666855014L);
        c(cie_x_integral<XYZ_31, T>(380, 780), 106.7136686555682046723277466L);
        c(cie_y_integral<XYZ_31, T>(380, 780), 106.9437893849319417309744979L);
        c(cie_z_integral<XYZ_31, T>(380, 780), 106.8063035972704729918887893L);
        c(cie_x_integral<XYZ_31, T>(480, 580), 28.6543035977093031216216843L);
        c(cie_y_integral<XYZ_31, T>(480, 580), 70.2596831086948870041508517L);
        c(cie_z_integral<XYZ_31, T>(480, 580), 14.2268442440640586217767044L);
        c(cie_x_integral<XYZ_31, T>(480, 680), 89.2257085372986290330338891L);
        c(cie_y_integral<XYZ_31, T>(480, 680), 103.855963307930482859594576L);
        c(cie_z_integral<XYZ_31, T>(480, 680), 14.2306992082538764220224972L);
        c(cie_x_integral<XYZ_31, T>(480, 780), 89.61844663757309625601663381L);
        c(cie_y_integral<XYZ_31, T>(480, 780), 104.1067971671280265676017252L);
        c(cie_z_integral<XYZ_31, T>(480, 780), 14.2306992090354015472257852L);
        c(cie_x_integral<XYZ_31, T>(580, 680), 60.5714049395893259114122047L);
        c(cie_y_integral<XYZ_31, T>(580, 680), 33.596280199235595855443724L);
        c(cie_z_integral<XYZ_31, T>(580, 680), 0.0038549641898178002457928L);
        c(cie_x_integral<XYZ_31, T>(580, 780), 60.9641430398637931343949495L);
        c(cie_y_integral<XYZ_31, T>(580, 780), 33.8471140584331395634508735L);
        c(cie_z_integral<XYZ_31, T>(580, 780), 0.0038549649713429254490808L);
        c(cie_x_integral<XYZ_31, T>(680, 780), 0.3927381002744672229827447L);
        c(cie_y_integral<XYZ_31, T>(680, 780), 0.2508338591975437080071495L);
        c(cie_z_integral<XYZ_31, T>(680, 780), 0.0000000007815251252032879332L);
}

/*
x[w1_, w2_] :=
 Integrate[
  0.398`30*Exp[-1250*Log[(x + 570.1`30)/1014]^2] +
   1.132`30*Exp[-234*Log[(1338 - x)/743.5`30]^2], {x, w1, w2}]
y[w1_, w2_] :=
 Integrate[1.011`30*Exp[-1/2*((x - 556.1`30)/46.14`30)^2], {x, w1, w2}]
z[w1_, w2_] :=
 Integrate[
  2.06`30*Exp[-32*Log[(x - 265.8`30)/180.4`30]^2], {x, w1, w2}]

For[i = 380, i < 780, i += 100,
 For[j = i + 100, j <= 780, j += 100,
  Print[StringTemplate["c(cie_x_integral<XYZ_64, T>(``, ``), ``L);"][
    i, j, x[i, j]]];
  Print[StringTemplate["c(cie_y_integral<XYZ_64, T>(``, ``), ``L);"][
    i, j, y[i, j]]];
  Print[StringTemplate["c(cie_z_integral<XYZ_64, T>(``, ``), ``L);"][
    i, j, z[i, j]]]
  ]]
*/

template <typename T>
void test_64(T precision)
{
        const auto c = [&](T a, T b)
        {
                check<T>(a, b, precision, 5e-9);
        };

        c(cie_x_integral<XYZ_64, T>(380, 480), 19.48847732828588252162093L);
        c(cie_y_integral<XYZ_64, T>(380, 480), 5.7847314073216769548408396L);
        c(cie_z_integral<XYZ_64, T>(380, 480), 104.921897804349332418615282L);
        c(cie_x_integral<XYZ_64, T>(380, 580), 54.89089498511375680712417L);
        c(cie_y_integral<XYZ_64, T>(380, 580), 81.5806128939646479680617563L);
        c(cie_z_integral<XYZ_64, T>(380, 580), 117.343615988990629042884287L);
        c(cie_x_integral<XYZ_64, T>(380, 680), 117.50005500535119745595017L);
        c(cie_y_integral<XYZ_64, T>(380, 680), 116.4964829125227628356972433L);
        c(cie_z_integral<XYZ_64, T>(380, 680), 117.344557478247753697744279L);
        c(cie_x_integral<XYZ_64, T>(380, 780), 117.849840154035804790415981L);
        c(cie_y_integral<XYZ_64, T>(380, 780), 116.9200625431827921713652154L);
        c(cie_z_integral<XYZ_64, T>(380, 780), 117.344557482254431577238072L);
        c(cie_x_integral<XYZ_64, T>(480, 580), 35.40241765682787428550324L);
        c(cie_y_integral<XYZ_64, T>(480, 580), 75.7958814866429710132209167L);
        c(cie_z_integral<XYZ_64, T>(480, 580), 12.421718184641296624269005L);
        c(cie_x_integral<XYZ_64, T>(480, 680), 98.01157767706531493432924L);
        c(cie_y_integral<XYZ_64, T>(480, 680), 110.7117515052010858808564037L);
        c(cie_z_integral<XYZ_64, T>(480, 680), 12.422659673898421279128996L);
        c(cie_x_integral<XYZ_64, T>(480, 780), 98.36136282574992226879505L);
        c(cie_y_integral<XYZ_64, T>(480, 780), 111.1353311358611152165243758L);
        c(cie_z_integral<XYZ_64, T>(480, 780), 12.42265967790509915862279L);
        c(cie_x_integral<XYZ_64, T>(580, 680), 62.609160020237440648826L);
        c(cie_y_integral<XYZ_64, T>(580, 680), 34.915870018558114867635487L);
        c(cie_z_integral<XYZ_64, T>(580, 680), 0.0009414892571246548599919L);
        c(cie_x_integral<XYZ_64, T>(580, 780), 62.95894516892204798329181L);
        c(cie_y_integral<XYZ_64, T>(580, 780), 35.3394496492181442033034591L);
        c(cie_z_integral<XYZ_64, T>(580, 780), 0.0009414932638025343537858L);
        c(cie_x_integral<XYZ_64, T>(680, 780), 0.34978514868460733446581L);
        c(cie_y_integral<XYZ_64, T>(680, 780), 0.4235796306600293356679721L);
        c(cie_z_integral<XYZ_64, T>(680, 780), 0.000000004006677879493793813L);
}

void test_integrals()
{
        LOG("Test XYZ integrals");

        test_31<float>(1e-3);
        test_31<double>(1e-12);
        test_31<long double>(1e-14);

        test_64<float>(1e-3);
        test_64<double>(1e-12);
        test_64<long double>(1e-14);

        LOG("Test XYZ integrals passed");
}

TEST_SMALL("XYZ Integrals", test_integrals)
}
}
