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

#include <src/color/samples/xyz_samples.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <vector>

namespace ns::color::samples
{
namespace
{
template <typename T>
void check_sum(const std::vector<T>& samples, const double min, const double max)
{
        if (samples.empty())
        {
                error("No samples");
        }

        double sum = 0;
        for (const double v : samples)
        {
                if (!(v >= 0))
                {
                        error("Sample " + to_string(v) + " is not positive and not zero");
                }
                sum += v;
        }

        if (!(sum >= min && sum <= max))
        {
                error("Sample sum " + to_string(sum) + " is not in the range [" + to_string(min) + ", " + to_string(max)
                      + "]");
        }
}

template <XYZ TYPE>
void test()
{
        constexpr double MIN = XYZ_SAMPLES_MIN_WAVELENGTH;
        constexpr double MAX = XYZ_SAMPLES_MAX_WAVELENGTH;

        static_assert(MIN < 400);
        static_assert(MAX > 700);

        check_sum(cie_x_samples<TYPE>(400, 700, 1), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(400, 700, 1), 0.99, 0.9999);
        check_sum(cie_z_samples<TYPE>(400, 700, 1), 0.99, 1.01);

        check_sum(cie_x_samples<TYPE>(400, 700, 60), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(400, 700, 60), 0.99, 0.9999);
        check_sum(cie_z_samples<TYPE>(400, 700, 60), 0.99, 1.01);

        check_sum(cie_x_samples<TYPE>(400, 700, 1000), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(400, 700, 1000), 0.99, 0.9999);
        check_sum(cie_z_samples<TYPE>(400, 700, 1000), 0.99, 1.01);

        check_sum(cie_x_samples<TYPE>(MIN, MAX, 1), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(MIN, MAX, 1), 1 - 1e-7, 1 + 1e-7);
        check_sum(cie_z_samples<TYPE>(MIN, MAX, 1), 0.99, 1.01);

        check_sum(cie_x_samples<TYPE>(MIN, MAX, 100), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(MIN, MAX, 100), 1 - 1e-7, 1 + 1e-7);
        check_sum(cie_z_samples<TYPE>(MIN, MAX, 100), 0.99, 1.01);

        check_sum(cie_x_samples<TYPE>(MIN, MAX, 1000), 0.99, 1.01);
        check_sum(cie_y_samples<TYPE>(MIN, MAX, 1000), 1 - 1e-7, 1 + 1e-7);
        check_sum(cie_z_samples<TYPE>(MIN, MAX, 1000), 0.99, 1.01);
}

void test_samples()
{
        LOG("Test XYZ samples");

        test<XYZ_31>();
        test<XYZ_64>();

        LOG("Test XYZ samples passed");
}

TEST_SMALL("XYZ Samples", test_samples)
}
}
