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

#include "simulator.h"

#include "move/move.h"
#include "move/move_filter_ukf_1.h"
#include "move/move_filter_ukf_2.h"
#include "position/position.h"
#include "position/position_estimation.h"
#include "position/position_filter_lkf_1.h"
#include "position/position_filter_lkf_2.h"
#include "position/position_variance.h"
#include "process/process.h"
#include "process/process_filter_ekf.h"
#include "process/process_filter_ukf.h"
#include "view/write.h"

#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/sort.h>
#include <src/test/test.h>

#include <cmath>
#include <optional>
#include <sstream>
#include <vector>

namespace ns::filter::test
{
namespace
{
template <typename T>
struct Config final
{
        static constexpr T POSITION_FILTER_VARIANCE_1 = square(1);
        static constexpr std::optional<T> POSITION_FILTER_GATE_1{10};
        static constexpr T POSITION_FILTER_VARIANCE_2 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_2{5};
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_TIME_DIFFERENCE = 1;
        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});
        static constexpr T POSITION_FILTER_RESET_DT = 10;
        static constexpr T POSITION_FILTER_LINEAR_DT = 2;

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(1.0);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr std::array PROCESS_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});
        static constexpr T PROCESS_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> PROCESS_FILTER_GATE{};

        static constexpr T MOVE_FILTER_POSITION_VARIANCE_1 = square(2.0);
        static constexpr T MOVE_FILTER_POSITION_VARIANCE_2 = square(1.0);
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array MOVE_FILTER_UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T MOVE_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> MOVE_FILTER_GATE{};

        static constexpr T DATA_CONNECT_INTERVAL = 10;
};

template <std::size_t N, typename T>
void write_to_file(
        const std::string_view annotation,
        const std::vector<Measurements<N, T>>& measurements,
        const std::vector<const std::vector<Position<N, T>>*>& positions,
        const std::vector<const std::vector<Process<T>>*>& processes,
        const std::vector<const std::vector<Move<T>>*>& moves)
{
        std::vector<view::Filter<N, T>> filters;

        const auto push = [&](const auto& f)
        {
                filters.push_back(
                        {.name = f.name(),
                         .color = f.color(),
                         .speed = f.speeds(),
                         .speed_p = f.speeds_p(),
                         .position = f.positions(),
                         .position_p = f.positions_p()});
        };

        const auto push_all = [&](const auto v)
        {
                for (const auto& p0 : v)
                {
                        for (const auto& p1 : *p0)
                        {
                                push(p1);
                        }
                }
        };

        push_all(positions);

        push_all(processes);

        push_all(moves);

        view::write_to_file(annotation, measurements, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <std::size_t N, typename T>
int compute_precision(const std::array<T, N>& data)
{
        std::optional<T> min;
        for (const T v : data)
        {
                ASSERT(v >= 0);
                if (!(v > 0))
                {
                        continue;
                }
                if (min)
                {
                        min = std::min(v, *min);
                        continue;
                }
                min = v;
        }
        if (!min)
        {
                return 0;
        }
        ASSERT(*min >= 1e-6L);
        return std::abs(std::floor(std::log10(*min)));
}

template <std::size_t N, typename T>
std::vector<PositionVariance<N, T>> create_position_variance()
{
        static constexpr T THETA{0};

        std::vector<PositionVariance<N, T>> res;

        res.emplace_back(
                "Variance LKF", color::RGB8(0, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                create_position_filter_lkf_2<N, T>(THETA, Config<T>::POSITION_FILTER_VARIANCE_2));

        return res;
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<Position<N, T>> create_positions()
{
        std::vector<Position<N, T>> res;

        const int precision = compute_precision(Config<T>::POSITION_FILTER_THETAS);

        const auto name = [&](const T theta)
        {
                const auto* const letter_theta = reinterpret_cast<const char*>(u8"\u03b8");
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "LKF " << ORDER << " (" << letter_theta << " " << theta << ")";
                return oss.str();
        };

        const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                ASSERT(i <= 4);

                static_assert(ORDER == 1 || ORDER == 2);

                if (ORDER == 1)
                {
                        res.emplace_back(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 200), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_1,
                                create_position_filter_lkf_1<N, T>(thetas[i], Config<T>::POSITION_FILTER_VARIANCE_1));
                }

                if (ORDER == 2)
                {
                        res.emplace_back(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_2,
                                create_position_filter_lkf_2<N, T>(thetas[i], Config<T>::POSITION_FILTER_VARIANCE_2));
                }
        }

        return res;
}

template <typename T>
std::vector<Process<T>> create_processes()
{
        const T process_pv = Config<T>::PROCESS_FILTER_POSITION_VARIANCE;
        const T process_av = Config<T>::PROCESS_FILTER_ANGLE_VARIANCE;
        const T process_arv = Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE;

        std::vector<Process<T>> res;

        res.emplace_back(
                "EKF", color::RGB8(0, 200, 0), Config<T>::PROCESS_FILTER_RESET_DT,
                create_process_filter_ekf<T>(process_pv, process_av, process_arv, Config<T>::PROCESS_FILTER_GATE));

        const int precision = compute_precision(Config<T>::PROCESS_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                const auto* const letter_alpha = reinterpret_cast<const char*>(u8"\u03b1");
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "UKF (" << letter_alpha << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::PROCESS_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);
                res.emplace_back(
                        name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0), Config<T>::PROCESS_FILTER_RESET_DT,
                        create_process_filter_ukf(
                                alphas[i], process_pv, process_av, process_arv, Config<T>::PROCESS_FILTER_GATE));
        }

        return res;
}

template <typename T, std::size_t ORDER>
std::vector<Move<T>> create_moves()
{
        const T move_av = Config<T>::MOVE_FILTER_ANGLE_VARIANCE;

        std::vector<Move<T>> res;

        const int precision = compute_precision(Config<T>::MOVE_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                const auto* const letter_alpha = reinterpret_cast<const char*>(u8"\u03b1");
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Move UKF " << ORDER << " (" << letter_alpha << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::MOVE_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);

                static_assert(ORDER == 1 || ORDER == 2);

                if (ORDER == 1)
                {
                        res.emplace_back(
                                name(alphas[i]), color::RGB8(0, 160 - 40 * i, 200), Config<T>::MOVE_FILTER_RESET_DT,
                                Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE,
                                create_move_filter_ukf_1(
                                        alphas[i], Config<T>::MOVE_FILTER_POSITION_VARIANCE_1, move_av));
                }

                if (ORDER == 2)
                {
                        res.emplace_back(
                                name(alphas[i]), color::RGB8(0, 160 - 40 * i, 100), Config<T>::MOVE_FILTER_RESET_DT,
                                Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE,
                                create_move_filter_ukf_2(
                                        alphas[i], Config<T>::MOVE_FILTER_POSITION_VARIANCE_2, move_av));
                }
        }

        return res;
}

template <std::size_t N, typename T>
void write_result(
        const std::string_view annotation,
        const std::vector<Measurements<N, T>>& measurements,
        const std::vector<PositionVariance<N, T>>& position_variance,
        const std::vector<const std::vector<Position<N, T>>*>& positions,
        const std::vector<const std::vector<Process<T>>*>& processes,
        const std::vector<const std::vector<Move<T>>*>& moves)
{
        write_to_file(annotation, measurements, positions, processes, moves);

        const auto log_consistency_string = [](const auto& p)
        {
                const std::string s = p.consistency_string();
                if (!s.empty())
                {
                        LOG(s);
                }
        };

        for (const auto& p : position_variance)
        {
                log_consistency_string(p);
        }

        const auto log = [&](const auto& v)
        {
                for (const auto& p0 : v)
                {
                        for (const auto& p1 : *p0)
                        {
                                log_consistency_string(p1);
                        }
                }
        };

        log(positions);

        log(processes);

        log(moves);
}

template <std::size_t N, typename T>
std::optional<Vector<N, T>> compute_variance(const std::vector<PositionVariance<N, T>>& positions)
{
        if (positions.size() == 1)
        {
                return positions.front().last_position_variance();
        }

        Vector<N, T> sum(0);
        std::size_t count = 0;
        for (const PositionVariance<N, T>& position : positions)
        {
                const auto& variance = position.last_position_variance();
                if (!variance)
                {
                        continue;
                }

                ++count;
                for (std::size_t i = 0; i < N; ++i)
                {
                        sum[i] += std::sqrt((*variance)[i]);
                }
        }

        if (count == 0)
        {
                return {};
        }

        sum /= static_cast<T>(count);
        for (std::size_t i = 0; i < N; ++i)
        {
                sum[i] = square(sum[i]);
        }
        return sum;
}

template <typename T>
void test_impl(const Track<2, T>& track)
{
        std::vector<Measurements<2, T>> measurements;

        std::vector<PositionVariance<2, T>> position_variance = create_position_variance<2, T>();
        std::vector<Position<2, T>> positions_1 = create_positions<2, T, 1>();
        std::vector<Position<2, T>> positions_2 = create_positions<2, T, 2>();
        std::vector<Process<T>> processes = create_processes<T>();
        std::vector<Move<T>> moves_1 = create_moves<T, 1>();
        std::vector<Move<T>> moves_2 = create_moves<T, 2>();

        PositionEstimation<T> position_estimation(
                Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_TIME_DIFFERENCE,
                Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE);

        const auto update = [&](const Measurements<2, T>& measurement)
        {
                for (auto& p : positions_2)
                {
                        p.update_position(measurement);
                }

                position_estimation.update(measurement, &std::as_const(positions_2));

                for (auto& p : positions_1)
                {
                        p.update_position(measurement);
                }

                for (auto& p : processes)
                {
                        p.update(measurement, std::as_const(position_estimation));
                }

                for (auto& m : moves_1)
                {
                        m.update(measurement, std::as_const(position_estimation));
                }

                for (auto& m : moves_2)
                {
                        m.update(measurement, std::as_const(position_estimation));
                }
        };

        for (Measurements<2, T> measurement : track)
        {
                measurements.push_back(measurement);

                for (auto& p : position_variance)
                {
                        p.update_position(measurement);
                }

                if (measurement.position && !measurement.position->variance)
                {
                        measurement.position->variance = compute_variance(position_variance);
                }

                update(measurement);
        }

        write_result(
                track.annotation(), measurements, position_variance, {&positions_1, &positions_2}, {&processes},
                {&moves_1, &moves_2});
}

template <typename T>
void test_impl()
{
        test_impl(track<2, T>());
}

void test()
{
        LOG("Test Filter 2D");
        LOG("---");
        test_impl<float>();
        LOG("---");
        test_impl<double>();
        LOG("---");
        test_impl<long double>();
        LOG("---");
        LOG("Test Filter 2D passed");
}

TEST_LARGE("Filter 2D", test)
}
}
