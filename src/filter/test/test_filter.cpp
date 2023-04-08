/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/exponent.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <cmath>
#include <fstream>
#include <random>
#include <vector>

namespace ns::filter
{
namespace
{
std::filesystem::path file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(name);
}

template <typename T>
struct ProcessData final
{
        T x;
        T z;

        friend std::string to_string(const ProcessData<T>& data)
        {
                return '(' + to_string(data.x) + ", " + to_string(data.z) + ')';
        }
};

template <typename T>
std::vector<ProcessData<T>> generate_random_data(
        const std::size_t count,
        const T velocity_mean,
        const T velocity_variance,
        const T measurement_variance,
        PCG& engine)
{
        std::normal_distribution<T> nd_v(velocity_mean, std::sqrt(velocity_variance));
        std::normal_distribution<T> nd_m(0, std::sqrt(measurement_variance));

        T x = 0;
        std::vector<ProcessData<T>> res;
        res.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                x += nd_v(engine);
                res.push_back({.x = x, .z = x + nd_m(engine)});
        }
        return res;
}

template <typename T>
void write_to_file(const std::string& file_name, const std::vector<ProcessData<T>>& data)
{
        std::ofstream file(file_path(file_name));

        for (const ProcessData<T>& d : data)
        {
                file << to_string(d) << '\n';
        }
}

template <typename T>
void test()
{
        const std::size_t count = 100;
        const T velocity_mean = 1;
        const T velocity_variance = power<2>(0.1);
        const T measurement_variance = power<2>(1);

        PCG engine;

        const std::vector<ProcessData<T>> data =
                generate_random_data<double>(count, velocity_mean, velocity_variance, measurement_variance, engine);

        write_to_file("filter.txt", data);
}

TEST_SMALL("Filter", test<double>)
}
}
