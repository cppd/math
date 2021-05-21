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

#include "../xyz_samples.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

namespace ns::color
{
namespace
{
void check_sum(const std::vector<float>& samples, double min, double max)
{
        if (samples.empty())
        {
                error("No samples");
        }
        double sum = 0;
        for (double v : samples)
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

void test_samples()
{
        LOG("Test XYZ integrals");

        check_sum(cie_x_samples(400, 700, 60), 0.99, 1.01);
        check_sum(cie_y_samples(400, 700, 60), 0.99, 1.01);
        check_sum(cie_z_samples(400, 700, 60), 0.99, 1.01);

        check_sum(cie_x_samples(380, 780, 100), 0.99, 1.01);
        check_sum(cie_y_samples(380, 780, 100), 0.99, 1.01);
        check_sum(cie_z_samples(380, 780, 100), 0.99, 1.01);

        LOG("Test XYZ integrals passed");
}

TEST_SMALL("XYZ Samples", test_samples)
}
}
