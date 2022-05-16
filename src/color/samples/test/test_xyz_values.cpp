/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../xyz_functions.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <sstream>

namespace ns::color
{
namespace
{
// clang-format off
constexpr std::array WAVES = std::to_array<double>
({
        380, 385, 390, 395, 400, 405, 410, 415, 420,
        425, 430, 435, 440, 445, 450, 455, 460, 465,
        470, 475, 480, 485, 490, 495, 500, 505, 510,
        515, 520, 525, 530, 535, 540, 545, 550, 555,
        560, 565, 570, 575, 580, 585, 590, 595, 600,
        605, 610, 615, 620, 625, 630, 635, 640, 645,
        650, 655, 660, 665, 670, 675, 680, 685, 690,
        695, 700, 705, 710, 715, 720, 725, 730, 735,
        740, 745, 750, 755, 760, 765, 770, 775, 780
});
constexpr std::array X_31 = std::to_array<double>
({
        0.001368, 0.002236, 0.004243, 0.007650, 0.014310, 0.023190,
        0.043510, 0.077630, 0.134380, 0.214770, 0.283900, 0.328500,
        0.348280, 0.348060, 0.336200, 0.318700, 0.290800, 0.251100,
        0.195360, 0.142100, 0.095640, 0.057950, 0.032010, 0.014700,
        0.004900, 0.002400, 0.009300, 0.029100, 0.063270, 0.109600,
        0.165500, 0.225750, 0.290400, 0.359700, 0.433450, 0.512050,
        0.594500, 0.678400, 0.762100, 0.842500, 0.916300, 0.978600,
        1.026300, 1.056700, 1.062200, 1.045600, 1.002600, 0.938400,
        0.854450, 0.751400, 0.642400, 0.541900, 0.447900, 0.360800,
        0.283500, 0.218700, 0.164900, 0.121200, 0.087400, 0.063600,
        0.046770, 0.032900, 0.022700, 0.015840, 0.011359, 0.008111,
        0.005790, 0.004109, 0.002899, 0.002049, 0.001440, 0.001000,
        0.000690, 0.000476, 0.000332, 0.000235, 0.000166, 0.000117,
        0.000083, 0.000059, 0.000042
});
constexpr std::array Y_31 = std::to_array<double>
({
        0.000039, 0.000064, 0.000120, 0.000217, 0.000396, 0.000640,
        0.001210, 0.002180, 0.004000, 0.007300, 0.011600, 0.016840,
        0.023000, 0.029800, 0.038000, 0.048000, 0.060000, 0.073900,
        0.090980, 0.112600, 0.139020, 0.169300, 0.208020, 0.258600,
        0.323000, 0.407300, 0.503000, 0.608200, 0.710000, 0.793200,
        0.862000, 0.914850, 0.954000, 0.980300, 0.994950, 1.000000,
        0.995000, 0.978600, 0.952000, 0.915400, 0.870000, 0.816300,
        0.757000, 0.694900, 0.631000, 0.566800, 0.503000, 0.441200,
        0.381000, 0.321000, 0.265000, 0.217000, 0.175000, 0.138200,
        0.107000, 0.081600, 0.061000, 0.044580, 0.032000, 0.023200,
        0.017000, 0.011920, 0.008210, 0.005723, 0.004102, 0.002929,
        0.002091, 0.001484, 0.001047, 0.000740, 0.000520, 0.000361,
        0.000249, 0.000172, 0.000120, 0.000085, 0.000060, 0.000042,
        0.000030, 0.000021, 0.000015
});
constexpr std::array Z_31 = std::to_array<double>
({
        0.006450, 0.010550, 0.020050, 0.036210, 0.067850, 0.110200,
        0.207400, 0.371300, 0.645600, 1.039050, 1.385600, 1.622960,
        1.747060, 1.782600, 1.772110, 1.744100, 1.669200, 1.528100,
        1.287640, 1.041900, 0.812950, 0.616200, 0.465180, 0.353300,
        0.272000, 0.212300, 0.158200, 0.111700, 0.078250, 0.057250,
        0.042160, 0.029840, 0.020300, 0.013400, 0.008750, 0.005750,
        0.003900, 0.002750, 0.002100, 0.001800, 0.001650, 0.001400,
        0.001100, 0.001000, 0.000800, 0.000600, 0.000340, 0.000240,
        0.000190, 0.000100, 0.000050, 0.000030, 0.000020, 0.000010,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000
});
constexpr std::array X_64 = std::to_array<double>
({
        0.000160, 0.000662, 0.002362, 0.007242, 0.019110, 0.043400,
        0.084736, 0.140638, 0.204492, 0.264737, 0.314679, 0.357719,
        0.383734, 0.386726, 0.370702, 0.342957, 0.302273, 0.254085,
        0.195618, 0.132349, 0.080507, 0.041072, 0.016172, 0.005132,
        0.003816, 0.015444, 0.037465, 0.071358, 0.117749, 0.172953,
        0.236491, 0.304213, 0.376772, 0.451584, 0.529826, 0.616053,
        0.705224, 0.793832, 0.878655, 0.951162, 1.014160, 1.074300,
        1.118520, 1.134300, 1.123990, 1.089100, 1.030480, 0.950740,
        0.856297, 0.754930, 0.647467, 0.535110, 0.431567, 0.343690,
        0.268329, 0.204300, 0.152568, 0.112210, 0.081261, 0.057930,
        0.040851, 0.028623, 0.019941, 0.013842, 0.009577, 0.006605,
        0.004553, 0.003145, 0.002175, 0.001506, 0.001045, 0.000727,
        0.000508, 0.000356, 0.000251, 0.000178, 0.000126, 0.000090,
        0.000065, 0.000046, 0.000033
});
constexpr std::array Y_64 = std::to_array<double>
({
        0.000017, 0.000072, 0.000253, 0.000769, 0.002004, 0.004509,
        0.008756, 0.014456, 0.021391, 0.029497, 0.038676, 0.049602,
        0.062077, 0.074704, 0.089456, 0.106256, 0.128201, 0.152761,
        0.185190, 0.219940, 0.253589, 0.297665, 0.339133, 0.395379,
        0.460777, 0.531360, 0.606741, 0.685660, 0.761757, 0.823330,
        0.875211, 0.923810, 0.961988, 0.982200, 0.991761, 0.999110,
        0.997340, 0.982380, 0.955552, 0.915175, 0.868934, 0.825623,
        0.777405, 0.720353, 0.658341, 0.593878, 0.527963, 0.461834,
        0.398057, 0.339554, 0.283493, 0.228254, 0.179828, 0.140211,
        0.107633, 0.081187, 0.060281, 0.044096, 0.031800, 0.022602,
        0.015905, 0.011130, 0.007749, 0.005375, 0.003718, 0.002565,
        0.001768, 0.001222, 0.000846, 0.000586, 0.000407, 0.000284,
        0.000199, 0.000140, 0.000098, 0.000070, 0.000050, 0.000036,
        0.000025, 0.000018, 0.000013
});
constexpr std::array Z_64 = std::to_array<double>
({
        0.000705, 0.002928, 0.010482, 0.032344, 0.086011, 0.197120,
        0.389366, 0.656760, 0.972542, 1.282500, 1.553480, 1.798500,
        1.967280, 2.027300, 1.994800, 1.900700, 1.745370, 1.554900,
        1.317560, 1.030200, 0.772125, 0.570060, 0.415254, 0.302356,
        0.218502, 0.159249, 0.112044, 0.082248, 0.060709, 0.043050,
        0.030451, 0.020584, 0.013676, 0.007918, 0.003988, 0.001091,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
        0.000000, 0.000000, 0.000000
});
// clang-format on

static_assert(std::is_sorted(std::cbegin(WAVES), std::cend(WAVES)));
static_assert(X_31.size() == WAVES.size());
static_assert(Y_31.size() == WAVES.size());
static_assert(Z_31.size() == WAVES.size());
static_assert(X_64.size() == WAVES.size());
static_assert(Y_64.size() == WAVES.size());
static_assert(Z_64.size() == WAVES.size());

template <typename T, typename F>
void check_non_negative(const std::string_view& name, const F& f)
{
        for (int i = 3800; i <= 7800; ++i)
        {
                const T w = static_cast<T>(i) / 10;
                const T v = f(w);
                if (!(v >= 0))
                {
                        std::ostringstream oss;
                        oss.precision(Limits<T>::max_digits10());
                        oss << name << ", approximation " << v << " is not non-negative for wave " << w;
                        error(oss.str());
                }
        }
}

template <typename T>
T check_abs(const std::string_view& name, const T& wave, const T& f, const T& tab, const T& max_error)
{
        const T abs_error = std::abs(f - tab);
        if (abs_error < max_error)
        {
                return abs_error;
        }
        std::ostringstream oss;
        oss.precision(Limits<T>::max_digits10());
        oss << name;
        oss << ", wavelength = " << wave;
        oss << ", f = " << f;
        oss << ", tab = " << tab;
        oss << ", error = " << abs_error;
        error(oss.str());
}

template <typename T>
void check_mean(const std::string_view& name, const T& mean, const T& max_error)
{
        if (mean < max_error)
        {
                return;
        }
        std::ostringstream oss;
        oss.precision(Limits<T>::max_digits10());
        oss << name << ", mean error = " << mean;
        error(oss.str());
}

template <typename T, typename F>
void check(
        const std::string_view& name,
        const F& f,
        const std::array<double, WAVES.size()>& data,
        const T& max_error,
        const T& max_mean_error)
{
        T sum = 0;
        for (std::size_t i = 0; i < WAVES.size(); ++i)
        {
                const T w = WAVES[i];
                sum += check_abs<T>(name, w, f(w), data[i], max_error);
        }
        check_mean<T>(name, sum / WAVES.size(), max_mean_error);

        check_non_negative<T>(name, f);
}

template <typename T>
void check()
{
        check<T>("X 31", cie_x_31<T>, X_31, 0.0139, 0.0049);
        check<T>("Y 31", cie_y_31<T>, Y_31, 0.0074, 0.0021);
        check<T>("Z 31", cie_z_31<T>, Z_31, 0.0222, 0.0021);

        check<T>("X 64", cie_x_64<T>, X_64, 0.0470, 0.0099);
        check<T>("Y 64", cie_y_64<T>, Y_64, 0.0254, 0.0092);
        check<T>("Z 64", cie_z_64<T>, Z_64, 0.0574, 0.0085);
}

void test()
{
        LOG("Test XYZ functions");

        check<float>();
        check<double>();
        check<long double>();

        LOG("Test XYZ functions passed");
}

TEST_SMALL("XYZ Functions", test)
}
}
