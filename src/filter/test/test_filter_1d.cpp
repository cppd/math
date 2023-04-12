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
[[nodiscard]] bool equal(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return true;
        }
        const T rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
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
        T filter;
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
        res += ", " + to_string(result.filter);
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

void test()
{
        using T = double;

        constexpr std::size_t N = 2;
        constexpr std::size_t M = 1;

        constexpr T DT = 1;
        constexpr T VELOCITY_MEAN = 1;
        constexpr T VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T MEASUREMENT_VARIANCE = power<2>(3);

        constexpr auto ENGINE_INIT = 11111;

        constexpr Vector<N, T> X(10, 5);
        constexpr Matrix<N, N, T> P{
                {500,  0},
                {  0, 50}
        };
        constexpr Matrix<N, N, T> F{
                {1, DT},
                {0,  1}
        };
        constexpr Matrix<M, N, T> H{
                {1, 0}
        };
        constexpr Matrix<M, M, T> R{{MEASUREMENT_VARIANCE}};
        constexpr Matrix<N, N, T> Q{discrete_white_noise<N, T>(DT, VELOCITY_VARIANCE)};

        constexpr std::size_t COUNT = 50;

        const std::vector<ProcessData<T>> process_data = generate_random_data<double>(
                COUNT, DT, VELOCITY_MEAN, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, PCG(ENGINE_INIT));

        Filter<N, M, T> filter;
        filter.set_x(X);
        filter.set_p(P);
        filter.set_f(F);
        filter.set_q(Q);
        filter.set_h(H);
        filter.set_r(R);

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                filter.predict();
                filter.update(Vector<M, T>(process.z));
                const T f_x = filter.x()[0];
                const T f_stddev = std::sqrt(filter.p()(0, 0));
                result_data.push_back({.filter = f_x, .standard_deviation = f_stddev});
        }

        write_to_file("filter.txt", process_data, result_data);

        if (!equal(result_data.back().filter, 48.481651625145673, 0.0)
            || !equal(result_data.back().standard_deviation, 1.4306605516486153, 0.0))
        {
                error("Filter test failed");
        }
}

TEST_SMALL("Filter", test)
}
}
