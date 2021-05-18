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
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <array>
#include <sstream>

namespace ns::color
{
namespace
{
struct Entry
{
        double w;
        double x;
        double y;
        double z;
};

// clang-format off
constexpr std::array DATA_31 = std::to_array<Entry>
({
        {380, 0.001368, 0.000039, 0.006450},
        {400, 0.014310, 0.000396, 0.067850},
        {420, 0.134380, 0.004000, 0.645600},
        {440, 0.348280, 0.023000, 1.747060},
        {460, 0.290800, 0.060000, 1.669200},
        {480, 0.095640, 0.139020, 0.812950},
        {500, 0.004900, 0.323000, 0.272000},
        {520, 0.063270, 0.710000, 0.078250},
        {540, 0.290400, 0.954000, 0.020300},
        {560, 0.594500, 0.995000, 0.003900},
        {580, 0.916300, 0.870000, 0.001650},
        {600, 1.062200, 0.631000, 0.000800},
        {620, 0.854450, 0.381000, 0.000190},
        {640, 0.447900, 0.175000, 0.000020},
        {660, 0.164900, 0.061000, 0.000000},
        {680, 0.046770, 0.017000, 0.000000},
        {700, 0.011359, 0.004102, 0.000000},
        {720, 0.002899, 0.001047, 0.000000},
        {740, 0.000690, 0.000249, 0.000000},
        {760, 0.000166, 0.000060, 0.000000},
        {780, 0.000042, 0.000015, 0.000000}
});
constexpr std::array DATA_64 = std::to_array<Entry>
({
        {380, 0.000160, 0.000017, 0.000705},
        {400, 0.019110, 0.002004, 0.086011},
        {420, 0.204492, 0.021391, 0.972542},
        {440, 0.383734, 0.062077, 1.967280},
        {460, 0.302273, 0.128201, 1.745370},
        {480, 0.080507, 0.253589, 0.772125},
        {500, 0.003816, 0.460777, 0.218502},
        {520, 0.117749, 0.761757, 0.060709},
        {540, 0.376772, 0.961988, 0.013676},
        {560, 0.705224, 0.997340, 0.000000},
        {580, 1.014160, 0.868934, 0.000000},
        {600, 1.123990, 0.658341, 0.000000},
        {620, 0.856297, 0.398057, 0.000000},
        {640, 0.431567, 0.179828, 0.000000},
        {660, 0.152568, 0.060281, 0.000000},
        {680, 0.040851, 0.015905, 0.000000},
        {700, 0.009577, 0.003718, 0.000000},
        {720, 0.002175, 0.000846, 0.000000},
        {740, 0.000508, 0.000199, 0.000000},
        {760, 0.000126, 0.000050, 0.000000},
        {780, 0.000033, 0.000013, 0.000000}
});
// clang-format on

template <XYZ xyz>
constexpr const auto& data()
{
        static_assert(xyz == XYZ_31 || xyz == XYZ_64);
        switch (xyz)
        {
        case XYZ_31:
                return DATA_31;
        case XYZ_64:
                return DATA_64;
        }
}

std::ostream& operator<<(std::ostream& os, XYZ xyz)
{
        switch (xyz)
        {
        case XYZ_31:
                return (os << "XYZ 31");
        case XYZ_64:
                return (os << "XYZ 64");
        }
        error_fatal("Unknown XYZ type " + std::to_string(static_cast<long long>(xyz)));
}

template <XYZ xyz, typename T>
void check_non_negative()
{
        for (int w = 3800; w <= 7800; ++w)
        {
                T t = T(w) / 10;
                T x = cie_x<xyz>(t);
                T y = cie_y<xyz>(t);
                T z = cie_z<xyz>(t);
                if (!(x >= 0 && y >= 0 && z >= 0))
                {
                        std::ostringstream oss;
                        oss.precision(limits<T>::max_digits10);
                        oss << xyz << ", approximation is not non-negative";
                        oss << ", x = " << x;
                        oss << ", y = " << y;
                        oss << ", z = " << z;
                        error(oss.str());
                }
        }
}

template <XYZ xyz, typename T>
T check(const char* name, T wave, T f, T tab, T max_error)
{
        T abs_error = std::abs(f - tab);
        if (!(abs_error < max_error))
        {
                std::ostringstream oss;
                oss.precision(limits<T>::max_digits10);
                oss << xyz;
                oss << ", wavelength = " << wave;
                oss << ", " << name << " = " << f;
                oss << ", tab = " << tab;
                oss << ", error = " << abs_error;
                error(oss.str());
        }
        return abs_error;
};

template <XYZ xyz, typename T>
void check_mean(const char* name, T mean, T max_error)
{
        if (!(mean < max_error))
        {
                std::ostringstream oss;
                oss.precision(limits<T>::max_digits10);
                oss << xyz << ", mean error " << name << " = " << mean;
                error(oss.str());
        }
};

template <XYZ xyz, typename T>
void check(T max_error_x, T max_error_y, T max_error_z, T max_mean_error_x, T max_mean_error_y, T max_mean_error_z)
{
        T sum_x = 0;
        T sum_y = 0;
        T sum_z = 0;

        for (const Entry& entry : data<xyz>())
        {
                sum_x += check<xyz, T>("x", entry.w, cie_x<xyz>(entry.w), entry.x, max_error_x);
                sum_y += check<xyz, T>("y", entry.w, cie_y<xyz>(entry.w), entry.y, max_error_y);
                sum_z += check<xyz, T>("z", entry.w, cie_z<xyz>(entry.w), entry.z, max_error_z);
        }

        check_mean<xyz, T>("x", sum_x / data<xyz>().size(), max_mean_error_x);
        check_mean<xyz, T>("y", sum_y / data<xyz>().size(), max_mean_error_y);
        check_mean<xyz, T>("z", sum_z / data<xyz>().size(), max_mean_error_z);

        check_non_negative<xyz, T>();
}

template <typename T>
void check()
{
        check<XYZ_31, T>(0.0110, 0.0074, 0.0140, 0.0050, 0.0021, 0.0022);
        check<XYZ_64, T>(0.0470, 0.0220, 0.0400, 0.0110, 0.0096, 0.0081);
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
