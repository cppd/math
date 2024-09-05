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

#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/filter/utility/allan_deviation.h>
#include <src/filter/utility/files.h>
#include <src/test/test.h>

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <ios>
#include <random>
#include <vector>

namespace ns::filter::utility::test
{
namespace
{
template <typename T>
void test_impl()
{
        const std::size_t count = 10'000;

        PCG engine;
        std::normal_distribution<T> nd;

        std::vector<T> data(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                data[i] = nd(engine);
        }

        std::ofstream file(test_file_path("filter_utility_allan_deviation_" + replace_space(type_name<T>()) + ".txt"));
        file << std::setprecision(Limits<T>::max_digits10());
        file << std::scientific;

        const T frequency = 1;
        const std::size_t output_count = 100;
        for (const AllanDeviation<T>& ad : allan_deviation(data, frequency, output_count))
        {
                file << ad.tau << ' ' << ad.deviation << '\n';
        }
}

void test()
{
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
}

TEST_SMALL("Allan Deviation", test)
}
}
