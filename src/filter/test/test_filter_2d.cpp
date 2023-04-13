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
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <cctype>
#include <cmath>
#include <fstream>
#include <random>
#include <vector>

namespace ns::filter
{
namespace
{
std::string replace_space(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return res;
}

std::filesystem::path file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(name);
}

template <typename T>
struct ProcessData final
{
        Vector<2, T> x;
        Vector<2, T> z;
};

template <typename T, typename Engine>
std::vector<ProcessData<T>> generate_random_data(
        const std::size_t count,
        const T dt,
        const T velocity_mean,
        const T velocity_variance,
        const T measurement_variance,
        Engine&& engine)
{
        std::normal_distribution<T> nd_v(velocity_mean, std::sqrt(velocity_variance));
        std::normal_distribution<T> nd_m(0, std::sqrt(measurement_variance));

        Vector<2, T> x{0, 0};
        std::vector<ProcessData<T>> res;
        res.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                x += Vector<2, T>(dt * nd_v(engine), dt * nd_v(engine));
                res.push_back({.x = x, .z = x + Vector<2, T>(nd_m(engine), nd_m(engine))});
        }
        return res;
}

template <typename T>
std::string make_string(const ProcessData<T>& process)
{
        std::string res;
        res += '(' + to_string(process.x[0]);
        res += ", " + to_string(process.x[1]);
        res += ", " + to_string(process.z[0]);
        res += ", " + to_string(process.z[1]);
        res += ')';
        return res;
}

template <typename T>
void write_to_file(const std::string& file_name, const std::vector<ProcessData<T>>& process)
{
        std::ofstream file(file_path(file_name));
        for (std::size_t i = 0; i < process.size(); ++i)
        {
                file << make_string(process[i]) << '\n';
        }
}

template <typename T>
void test_impl()
{
        constexpr T DT = 1;
        constexpr T VELOCITY_MEAN = 1;
        constexpr T VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T MEASUREMENT_VARIANCE = power<2>(3);

        constexpr std::size_t COUNT = 1000;

        const std::vector<ProcessData<T>> process_data =
                generate_random_data<T>(COUNT, DT, VELOCITY_MEAN, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, PCG());

        write_to_file("filter_2d_" + replace_space(type_name<T>()) + ".txt", process_data);
}

void test()
{
        LOG("Test Filter 2D");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Filter 2D passed");
}

TEST_SMALL("Filter 2D", test)
}
}
