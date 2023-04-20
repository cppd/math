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

#include "../filter.h"
#include "../models.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <random>
#include <unordered_map>
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
void compare(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return;
        }

        const T abs = std::abs(a - b);
        if (!(abs < precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b) + "; absolute " + to_string(abs)
                      + "; required precision " + to_string(precision));
        }
}

template <typename T>
struct ProcessData final
{
        T x;
        T z;
};

template <typename T>
struct ResultData final
{
        T x;
        T standard_deviation;
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

        T x = 0;
        std::vector<ProcessData<T>> res;
        res.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                x += dt * nd_v(engine);
                res.push_back({.x = x, .z = x + nd_m(engine)});
        }
        return res;
}

template <typename T>
std::string make_string(const ProcessData<T>& process, const ResultData<T>& result)
{
        std::string res;
        res += '(' + to_string(process.x);
        res += ", " + to_string(process.z);
        res += ", " + to_string(result.x);
        res += ", " + to_string(result.standard_deviation);
        res += ')';
        return res;
}

template <typename T>
void write_to_file(
        const std::string& file_name,
        const std::vector<ProcessData<T>>& process,
        const std::vector<ResultData<T>>& result)
{
        ASSERT(process.size() == result.size());

        std::ofstream file(file_path(file_name));
        for (std::size_t i = 0; i < process.size(); ++i)
        {
                file << make_string(process[i], result[i]) << '\n';
        }
}

std::string distribution_to_string(const std::unordered_map<int, unsigned>& distribution)
{
        std::string res;
        for (const auto& [k, v] : std::map{distribution.cbegin(), distribution.cend()})
        {
                if (!res.empty())
                {
                        res += '\n';
                }
                res += to_string(k) + ":" + to_string(v);
        }
        return res;
}

template <std::size_t N>
void check_distribution(
        std::unordered_map<int, unsigned> distribution,
        const std::array<unsigned, N>& expected_distribution)
{
        static_assert(N > 0);

        if (distribution.empty())
        {
                error("Filter distribution is empty");
        }

        const auto [min, max] = std::minmax_element(
                distribution.cbegin(), distribution.cend(),
                [](const auto& a, const auto& b)
                {
                        return a.first < b.first;
                });
        if (!(static_cast<std::size_t>(std::abs(min->first)) < expected_distribution.size()
              && static_cast<std::size_t>(std::abs(max->first)) < expected_distribution.size()))
        {
                error("Filter distribution 1 error\n" + distribution_to_string(distribution));
        }

        if (!(distribution[0] > expected_distribution[0]))
        {
                error("Filter distribution 2 error\n" + distribution_to_string(distribution));
        }
        for (std::size_t i = 1; i < expected_distribution.size(); ++i)
        {
                const int index = i;
                if (!(distribution[index] <= expected_distribution[index]
                      && distribution[-index] <= expected_distribution[index]))
                {
                        error("Filter distribution 3 error\n" + distribution_to_string(distribution));
                }
        }
}

template <typename T>
void test_impl()
{
        constexpr std::size_t N = 2;
        constexpr std::size_t M = 1;

        constexpr T DT = 1;
        constexpr T VELOCITY_MEAN = 1;
        constexpr T VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T MEASUREMENT_VARIANCE = power<2>(3);

        constexpr Vector<N, T> X(10, 5);
        constexpr Matrix<N, N, T> P{
                {500,  0},
                {  0, 50}
        };
        constexpr Matrix<N, N, T> F{
                {1, DT},
                {0,  1}
        };
        constexpr Matrix<N, N, T> Q{discrete_white_noise<N, T>(DT, VELOCITY_VARIANCE)};

        constexpr Matrix<M, N, T> H{
                {1, 0}
        };
        constexpr Matrix<N, M, T> H_T = H.transposed();
        constexpr Matrix<M, M, T> R{{MEASUREMENT_VARIANCE}};

        constexpr std::size_t COUNT = 1000;

        const std::vector<ProcessData<T>> process_data =
                generate_random_data<T>(COUNT, DT, VELOCITY_MEAN, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, PCG());

        Filter<N, T> filter;
        filter.set_x(X);
        filter.set_p(P);
        filter.set_f(F);
        filter.set_q(Q);

        std::unordered_map<int, unsigned> distribution;

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                filter.predict();
                filter.update(H, H_T, R, Vector<M, T>(process.z));

                const T f_x = filter.x()[0];
                const T f_stddev = std::sqrt(filter.p()(0, 0));

                result_data.push_back({.x = f_x, .standard_deviation = f_stddev});
                ++distribution[static_cast<int>((f_x - process.x) / f_stddev)];
        }

        write_to_file("filter_1d_" + replace_space(type_name<T>()) + ".txt", process_data, result_data);

        compare(result_data.back().standard_deviation, T{1.4306576889002234962L}, T{0});
        compare(process_data.back().x, result_data.back().x, 5 * result_data.back().standard_deviation);

        constexpr std::array EXPECTED_DISTRIBUTION = std::to_array<unsigned>({610, 230, 60, 15, 7, 2, 0, 0, 0, 0});
        check_distribution(distribution, EXPECTED_DISTRIBUTION);
}

void test()
{
        LOG("Test Filter 1D");
        test_impl<float>();
        test_impl<double>();
        test_impl<long double>();
        LOG("Test Filter 1D passed");
}

TEST_SMALL("Filter 1D", test)
}
}
