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

#include <src/com/exponent.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
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

template <std::size_t N, typename T>
struct ProcessData final
{
        std::vector<Vector<N, T>> track;
        std::vector<Vector<N, T>> measurements;
};

template <std::size_t N, typename T>
struct ResultData final
{
        std::vector<Vector<N, T>> filter;
};

template <std::size_t N, typename T, typename Engine>
ProcessData<N, T> generate_random_data(
        const std::size_t count,
        const T dt,
        const T velocity_mean,
        const T velocity_variance,
        const T measurement_variance,
        Engine&& engine)
{
        std::normal_distribution<T> nd_v(velocity_mean, std::sqrt(velocity_variance));
        std::normal_distribution<T> nd_m(0, std::sqrt(measurement_variance));

        Vector<N, T> x(0);
        ProcessData<N, T> res;
        res.track.reserve(count);
        res.measurements.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        x[n] += dt * nd_v(engine);
                }
                res.track.push_back(x);
                res.measurements.push_back(x);
                for (std::size_t n = 0; n < N; ++n)
                {
                        res.measurements.back()[n] += nd_m(engine);
                }
        }
        return res;
}

template <std::size_t N, typename T>
void write_to_file(const std::string& file_name, const ProcessData<N, T>& process, const ResultData<N, T>& result)
{
        std::ofstream file(file_path(file_name));
        file.precision(Limits<T>::max_digits10());

        const auto out = [&](const Vector<N, T>& v)
        {
                static_assert(N > 0);
                file << '(';
                file << v[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        file << ", " << v[i];
                }
                file << ")\n";
        };

        file << "\"track\"\n";
        for (const Vector<N, T>& v : process.track)
        {
                out(v);
        }

        file << "\"measurements\"\n";
        for (const Vector<N, T>& v : process.measurements)
        {
                out(v);
        }

        file << "\"filter\"\n";
        for (const Vector<N, T>& v : result.filter)
        {
                out(v);
        }
}

template <typename T>
void test_impl()
{
        constexpr std::size_t N = 4;
        constexpr std::size_t M = 2;

        constexpr T DT = 1;
        constexpr T VELOCITY_MEAN = 1;
        constexpr T VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T MEASUREMENT_VARIANCE = power<2>(3);

        constexpr Vector<N, T> X(10, 5, 10, 5);
        constexpr Matrix<N, N, T> P{
                {500,  0,   0,  0},
                {  0, 50,   0,  0},
                {  0,  0, 500,  0},
                {  0,  0,   0, 50}
        };
        constexpr Matrix<N, N, T> F{
                {1, DT, 0,  0},
                {0,  1, 0,  0},
                {0,  0, 1, DT},
                {0,  0, 0,  1}
        };
        constexpr Matrix<M, N, T> H{
                {1, 0, 0, 0},
                {0, 0, 1, 0}
        };
        constexpr Matrix<M, M, T> R{
                {MEASUREMENT_VARIANCE,                    0},
                {                   0, MEASUREMENT_VARIANCE}
        };
        constexpr Matrix<N, N, T> Q = []()
        {
                const auto m = discrete_white_noise<N / 2, T>(DT, VELOCITY_VARIANCE);
                return block_diagonal(std::array{m, m});
        }();

        constexpr std::size_t COUNT = 1000;

        const ProcessData process =
                generate_random_data<2, T>(COUNT, DT, VELOCITY_MEAN, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, PCG());

        Filter<N, M, T> filter;
        filter.set_x(X);
        filter.set_p(P);
        filter.set_f(F);
        filter.set_q(Q);
        filter.set_h(H);
        filter.set_r(R);

        ResultData<2, T> result;
        result.filter.reserve(process.measurements.size());
        for (const Vector<M, T>& z : process.measurements)
        {
                filter.predict();
                filter.update(z);

                result.filter.push_back({filter.x()[0], filter.x()[2]});
        }

        write_to_file("filter_2d_" + replace_space(type_name<T>()) + ".txt", process, result);
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
