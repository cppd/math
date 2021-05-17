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

namespace ns::color
{
namespace
{
template <typename T>
void check_31(T w, T x, T y, T z)
{
        const auto check = [&w](const char* name, T f, T tab)
        {
                T abs_error = std::abs(f - tab);
                if (!(abs_error < T(0.01)))
                {
                        error("XYZ-31, wavelength = " + to_string(w) + ", " + name + " = " + to_string(f)
                              + ", tab = " + to_string(tab) + ", error = " + to_string(abs_error));
                }
        };

        check("x", x_31(w), x);
        check("y", y_31(w), y);
        check("z", z_31(w), z);
}

template <typename T>
void check_31()
{
        check_31<T>(380, 0.001368, 0.000039, 0.006450);
        check_31<T>(400, 0.014310, 0.000396, 0.067850);
        check_31<T>(450, 0.336200, 0.038000, 1.772110);
        check_31<T>(500, 0.004900, 0.323000, 0.272000);
        check_31<T>(550, 0.433450, 0.994950, 0.008750);
        check_31<T>(600, 1.062200, 0.631000, 0.000800);
        check_31<T>(650, 0.283500, 0.107000, 0.000000);
        check_31<T>(700, 0.011359, 0.004102, 0.000000);
        check_31<T>(750, 0.000332, 0.000120, 0.000000);
        check_31<T>(780, 0.000042, 0.000015, 0.000000);

        for (int w = 3800; w <= 7800; ++w)
        {
                T t = T(w) / 10;
                T x = x_31(t);
                T y = y_31(t);
                T z = z_31(t);
                if (!(x >= 0 && y >= 0 && z >= 0))
                {
                        error("XYZ-31, approximation is not non-negative, x = " + to_string(x) + ", y = " + to_string(y)
                              + ", z = " + to_string(z));
                }
        }
}

void test_31()
{
        LOG("Test XYZ-31");

        check_31<float>();
        check_31<double>();
        check_31<long double>();

        LOG("Test XYZ-31 passed");
}

TEST_SMALL("XYZ-31", test_31)
}
}
