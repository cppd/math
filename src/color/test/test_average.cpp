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

#include "../average.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <array>
#include <cmath>

namespace ns::color
{
namespace
{
void check(const std::vector<float>& a, const std::vector<float>& b)
{
        if (a.size() != b.size())
        {
                error("Size error " + to_string(a.size()) + ", " + to_string(b.size()));
        }

        for (std::size_t i = 0; i < a.size(); ++i)
        {
                if (!(std::abs(a[i] - b[i]) < 1e-5))
                {
                        error(to_string(a[i]) + " and " + to_string(b[i]) + " are not equal");
                }
        }
}

void test()
{
        LOG("Test average samples");

        constexpr std::array<double, 3> waves{2, 4, 6};
        constexpr std::array<double, 3> samples{1, 1, 1};

        check(average(waves, samples, 0, 10, 4), {0.2, 1, 0.4, 0});
        check(average(waves, samples, 4, 6, 3), {1, 1, 1});
        check(average(waves, samples, 6, 8, 3), {0, 0, 0});
        check(average(waves, samples, 0, 2, 3), {0, 0, 0});
        check(average(waves, samples, 0, 2.5, 5), {0, 0, 0, 0, 1});
        check(average(waves, samples, 5.5, 8, 5), {1, 0, 0, 0, 0});

        LOG("Test average samples passed");
}

TEST_SMALL("Average Samples", test)
}
}
