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

#include "../consistency.h"
#include "../ekf.h"
#include "../models.h"
#include "../sigma_points.h"
#include "../ukf.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
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

void check_distribution(
        std::unordered_map<int, unsigned> distribution,
        const std::vector<unsigned>& expected_distribution)
{
        if (distribution.empty())
        {
                error("Filter distribution is empty");
        }

        if (expected_distribution.empty())
        {
                error("Filter expected distribution is empty");
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

        if (!(distribution[0] >= expected_distribution[0]))
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

struct Add final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a + b;
        }
};

struct Residual final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a - b;
        }
};

template <typename T, bool INF>
class TestEkf
{
        static constexpr std::optional<T> GATE{};
        static constexpr bool NORMALIZED_INNOVATION{true};
        static constexpr bool LIKELIHOOD{true};
        static constexpr std::optional<T> THETA{INF ? 0.01L : std::optional<T>()};

        const T dt_;

        const Matrix<2, 2, T> q_;
        const Matrix<1, 1, T> r_;

        Ekf<2, T> filter_;

public:
        using Type = T;

        TestEkf(const std::type_identity_t<T> dt,
                const std::type_identity_t<T> process_variance,
                const std::type_identity_t<T> measurement_variance,
                const Vector<2, T>& x,
                const Matrix<2, 2, T>& p)
                : dt_(dt),
                  q_(discrete_white_noise<2, T>(dt, process_variance)),
                  r_({{measurement_variance}}),
                  filter_(x, p)
        {
        }

        void process(const T measurement)
        {
                // x[0] = x[0] + dt * x[1]
                // x[1] = x[1]
                // Jacobian matrix
                //  1 dt
                //  0  1
                const auto f = [&](const Vector<2, T>& x)
                {
                        return Vector<2, T>(x[0] + dt_ * x[1], x[1]);
                };
                const auto f_jacobian = [&](const Vector<2, T>& /*x*/)
                {
                        return Matrix<2, 2, T>{
                                {1, dt_},
                                {0,   1}
                        };
                };

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

                filter_.predict(f, f_jacobian, q_);
                filter_.update(
                        h, h_jacobian, r_, Vector<1, T>(measurement), Add(), Residual(), GATE, THETA,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] T x() const
        {
                return filter_.x()[0];
        }

        [[nodiscard]] T variance() const
        {
                return filter_.p()(0, 0);
        }

        static std::string name()
        {
                if (!INF)
                {
                        return "EKF";
                }
                return "EXTENDED_H_INFINITY";
        }
};

template <typename T>
class TestUkf
{
        static constexpr std::optional<T> GATE{};
        static constexpr bool LIKELIHOOD{true};

        static constexpr T ALPHA = 0.1;
        static constexpr T BETA = 2; // 2 for Gaussian
        static constexpr T KAPPA = 1; // 3 − N

        const T dt_;

        const Matrix<2, 2, T> q_;
        const Matrix<1, 1, T> r_;

        Ukf<2, T, SigmaPoints<2, T>> filter_;

public:
        using Type = T;

        TestUkf(const std::type_identity_t<T> dt,
                const std::type_identity_t<T> process_variance,
                const std::type_identity_t<T> measurement_variance,
                const Vector<2, T>& x,
                const Matrix<2, 2, T>& p)
                : dt_(dt),
                  q_(discrete_white_noise<2, T>(dt, process_variance)),
                  r_({{measurement_variance}}),
                  filter_({ALPHA, BETA, KAPPA}, x, p)
        {
        }

        void process(const T measurement)
        {
                // x[0] = x[0] + dt * x[1]
                // x[1] = x[1]
                const auto f = [&](const Vector<2, T>& x)
                {
                        return Vector<2, T>(x[0] + dt_ * x[1], x[1]);
                };

                // measurement = x[0]
                const auto h = [](const Vector<2, T>& x)
                {
                        return Vector<1, T>(x[0]);
                };

                filter_.predict(f, q_);
                filter_.update(h, r_, Vector<1, T>(measurement), Add(), Residual(), GATE, LIKELIHOOD);
        }

        [[nodiscard]] T x() const
        {
                return filter_.x()[0];
        }

        [[nodiscard]] T variance() const
        {
                return filter_.p()(0, 0);
        }

        static std::string name()
        {
                return "UKF";
        }
};

template <typename Filter>
void test_impl(
        const typename Filter::Type precision,
        const typename Filter::Type expected_deviation,
        const typename Filter::Type deviation_count,
        const std::vector<unsigned>& expected_distribution)
{
        using T = typename Filter::Type;

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

        Filter filter(DT, VELOCITY_VARIANCE, MEASUREMENT_VARIANCE, X, P);

        std::unordered_map<int, unsigned> distribution;

        NormalizedSquared<1, T> nees;

        std::vector<ResultData<T>> result_data;
        result_data.reserve(process_data.size());
        for (const ProcessData<T>& process : process_data)
        {
                filter.process(process.z);

                const T x = filter.x();
                const T variance = filter.variance();
                const T stddev = std::sqrt(variance);

                result_data.push_back({.x = x, .standard_deviation = stddev});
                ++distribution[static_cast<int>((x - process.x) / stddev)];

                nees.add(process.x - x, variance);
        }

        write_to_file(
                "filter_" + to_lower(filter.name()) + "_1d_" + replace_space(type_name<T>()) + ".txt", process_data,
                result_data);

        compare(result_data.back().standard_deviation, expected_deviation, precision);
        compare(process_data.back().x, result_data.back().x, deviation_count * result_data.back().standard_deviation);

        if (!(nees.average() < T{1.35}))
        {
                error(nees.check_string());
        }

        check_distribution(distribution, expected_distribution);
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision)
{
        const std::vector<unsigned> distribution = {580, 230, 60, 16, 7, 3, 0, 0, 0, 0};
        test_impl<TestEkf<T, false>>(precision, 1.4306576889002234962L, 5, distribution);
        test_impl<TestEkf<T, true>>(precision, 1.43098764352003224212L, 5, distribution);
        test_impl<TestUkf<T>>(precision, 1.43670888967218343853L, 5, distribution);
}

void test()
{
        LOG("Test Filter 1D");
        test_impl<float>(1e-3);
        test_impl<double>(1e-12);
        test_impl<long double>(1e-15);
        LOG("Test Filter 1D passed");
}

TEST_SMALL("Filter 1D", test)
}
}
