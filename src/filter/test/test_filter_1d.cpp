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

#include "utility.h"

#include "../filter.h"
#include "../models.h"
#include "../nees.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <cmath>
#include <fstream>
#include <map>
#include <random>
#include <unordered_map>
#include <vector>

namespace ns::filter::test
{
namespace
{
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

        std::ofstream file(test_file_path(file_name));
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
class TestFilter
{
        const Matrix<2, 2, T> f_;
        const Matrix<2, 2, T> f_t_ = f_.transposed();
        const Matrix<2, 2, T> q_;

        const Matrix<1, 2, T> h_{
                {1, 0}
        };
        const Matrix<2, 1, T> h_t_ = h_.transposed();
        const Matrix<1, 1, T> r_;

        Filter<2, T> filter_;

public:
        TestFilter(
                const std::type_identity_t<T> dt,
                const std::type_identity_t<T> process_variance,
                const std::type_identity_t<T> measurement_variance,
                const Vector<2, T>& x,
                const Matrix<2, 2, T> p)
                : f_{{1, dt}, {0, 1}},
                  q_(discrete_white_noise<2, T>(dt, process_variance)),
                  r_({{measurement_variance}}),
                  filter_(x, p)
        {
        }

        void process(const T measurement)
        {
                filter_.predict(f_, f_t_, q_);
                filter_.update(h_, h_t_, r_, Vector<1, T>(measurement));
        }

        void process_ext(const T measurement)
        {
                // measurement = x[0]
                // Jacobian matrix
                //  1 0
                const auto h = [](const Vector<2, T>& x)
                {
                        return Vector<1, T>(x[0]);
                };
                const auto h_jacobian = [](const Vector<2, T>& /*x*/)
                {
                        return Matrix<1, 2, T>{
                                {1, 0}
                        };
                };
                filter_.predict(f_, f_t_, q_);
                filter_.update(h, h_jacobian, r_, Vector<1, T>(measurement));
        }

        [[nodiscard]] T x() const
        {
                return filter_.x()[0];
        }

        [[nodiscard]] T variance() const
        {
                return filter_.p()(0, 0);
        }
};

template <typename T, bool EXT>
void test_impl()
{
        constexpr T DT = 1;
        constexpr T VELOCITY_MEAN = 1;
        constexpr T VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T MEASUREMENT_VARIANCE = power<2>(3);

        constexpr std::size_t COUNT = 1000;

        const std::vector<ProcessData<T>> process_data =
                generate_random_data<T>(COUNT, DT, VELOCITY_MEAN, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, PCG());

        constexpr Vector<2, T> X(10, 5);
        constexpr Matrix<2, 2, T> P{
                {500,  0},
                {  0, 50}
        };

        TestFilter<T> filter(DT, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, X, P);

        std::unordered_map<int, unsigned> distribution;

        std::vector<Vector<1, T>> values;
        values.reserve(process_data.size());
        std::vector<Vector<1, T>> estimates;
        estimates.reserve(process_data.size());
        std::vector<Matrix<1, 1, T>> covariances;
        covariances.reserve(process_data.size());

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                if constexpr (EXT)
                {
                        filter.process_ext(process.z);
                }
                else
                {
                        filter.process(process.z);
                }

                const T x = filter.x();
                const T variance = filter.variance();
                const T stddev = std::sqrt(variance);

                result_data.push_back({.x = x, .standard_deviation = stddev});
                ++distribution[static_cast<int>((x - process.x) / stddev)];

                values.emplace_back(process.x);
                estimates.emplace_back(x);
                covariances.push_back(Matrix<1, 1, T>{{variance}});
        }

        write_to_file("filter_1d_" + replace_space(type_name<T>()) + ".txt", process_data, result_data);

        compare(result_data.back().standard_deviation, T{1.4306576889002234962L}, T{0});
        compare(process_data.back().x, result_data.back().x, 5 * result_data.back().standard_deviation);

        const T nees = nees_average(values, estimates, covariances);
        if (!(nees < T{1.2}))
        {
                error(std::string("NEES average <") + type_name<T>() + "> = " + to_string(nees)
                      + "; 1 degree of freedom; failed");
        }

        constexpr std::array EXPECTED_DISTRIBUTION = std::to_array<unsigned>({610, 230, 60, 15, 7, 2, 0, 0, 0, 0});
        check_distribution(distribution, EXPECTED_DISTRIBUTION);
}

template <typename T>
void test_impl()
{
        test_impl<T, false>();
        test_impl<T, true>();
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
