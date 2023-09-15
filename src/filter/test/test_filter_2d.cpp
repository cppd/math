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
#include "string.h"

#include "move/move_1_0.h"
#include "move/move_1_1.h"
#include "move/move_2_1.h"
#include "position/position_0.h"
#include "position/position_1.h"
#include "position/position_2.h"
#include "position/position_estimation.h"
#include "position/position_variance.h"
#include "process/process_ekf.h"
#include "process/process_ukf.h"
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
        static constexpr T POSITION_FILTER_VARIANCE_0 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_0{};

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

        static constexpr T MOVE_FILTER_POSITION_VARIANCE_1_0 = square(2.0);
        static constexpr T MOVE_FILTER_POSITION_VARIANCE_1_1 = square(2.0);
        static constexpr T MOVE_FILTER_POSITION_VARIANCE_2_1 = square(1.0);
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_1_0 = square(degrees_to_radians(0.2));
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_1_1 = square(degrees_to_radians(0.001));
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_2_1 = square(degrees_to_radians(0.001));
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
        const std::vector<const std::vector<std::unique_ptr<position::Position<N, T>>>*>& positions,
        const std::vector<const std::vector<std::unique_ptr<process::Process<T>>>*>& processes,
        const std::vector<const std::vector<std::unique_ptr<move::Move<T>>>*>& moves)
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
                                push(*p1);
                        }
                }
        };

        push_all(positions);

        push_all(processes);

        push_all(moves);

        view::write_to_file(annotation, measurements, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <std::size_t N, typename T>
std::vector<position::PositionVariance<N, T>> create_position_variance()
{
        std::vector<position::PositionVariance<N, T>> res;

        res.emplace_back(
                "Variance LKF", color::RGB8(0, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                Config<T>::POSITION_FILTER_VARIANCE_2);

        return res;
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<std::unique_ptr<position::Position<N, T>>> create_positions()
{
        std::vector<std::unique_ptr<position::Position<N, T>>> res;

        const int precision = compute_string_precision(Config<T>::POSITION_FILTER_THETAS);

        const auto name = [&](const T theta)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "LKF " << ORDER << " (" << THETA << " " << theta << ")";
                return oss.str();
        };

        const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                ASSERT(i <= 4);

                static_assert(ORDER >= 0 && ORDER <= 2);

                if (ORDER == 0)
                {
                        res.emplace_back(std::make_unique<position::Position0<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 100, 200),
                                Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_LINEAR_DT,
                                Config<T>::POSITION_FILTER_GATE_0, thetas[i], Config<T>::POSITION_FILTER_VARIANCE_0));
                }

                if (ORDER == 1)
                {
                        res.emplace_back(std::make_unique<position::Position1<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 200), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_1, thetas[i],
                                Config<T>::POSITION_FILTER_VARIANCE_1));
                }

                if (ORDER == 2)
                {
                        res.emplace_back(std::make_unique<position::Position2<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_2, thetas[i],
                                Config<T>::POSITION_FILTER_VARIANCE_2));
                }
        }

        return res;
}

template <typename T>
std::vector<std::unique_ptr<process::Process<T>>> create_processes()
{
        const T process_pv = Config<T>::PROCESS_FILTER_POSITION_VARIANCE;
        const T process_av = Config<T>::PROCESS_FILTER_ANGLE_VARIANCE;
        const T process_arv = Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE;

        std::vector<std::unique_ptr<process::Process<T>>> res;

        res.push_back(std::make_unique<process::ProcessEkf<T>>(
                "EKF", color::RGB8(0, 200, 0), Config<T>::PROCESS_FILTER_RESET_DT, Config<T>::PROCESS_FILTER_GATE,
                process_pv, process_av, process_arv, Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE));

        const int precision = compute_string_precision(Config<T>::PROCESS_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "UKF (" << ALPHA << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::PROCESS_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);
                res.push_back(std::make_unique<process::ProcessUkf<T>>(
                        name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0), Config<T>::PROCESS_FILTER_RESET_DT,
                        Config<T>::PROCESS_FILTER_GATE, alphas[i], process_pv, process_av, process_arv,
                        Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE));
        }

        return res;
}

template <typename T, std::size_t ORDER_P, std::size_t ORDER_A>
std::vector<std::unique_ptr<move::Move<T>>> create_moves()
{
        std::vector<std::unique_ptr<move::Move<T>>> res;

        const int precision = compute_string_precision(Config<T>::MOVE_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Move UKF " << ORDER_P << '.' << ORDER_A << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::MOVE_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);

                static_assert((ORDER_P == 1 && (ORDER_A == 0 || ORDER_A == 1)) || (ORDER_P == 2 && ORDER_A == 1));

                if (ORDER_P == 1 && ORDER_A == 0)
                {
                        res.push_back(std::make_unique<move::Move10<T>>(
                                name(alphas[i]), color::RGB8(0, 160 - 40 * i, 250), Config<T>::MOVE_FILTER_RESET_DT,
                                Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE,
                                alphas[i], Config<T>::MOVE_FILTER_POSITION_VARIANCE_1_0,
                                Config<T>::MOVE_FILTER_ANGLE_VARIANCE_1_0));
                }

                if (ORDER_P == 1 && ORDER_A == 1)
                {
                        res.push_back(std::make_unique<move::Move11<T>>(
                                name(alphas[i]), color::RGB8(0, 160 - 40 * i, 150), Config<T>::MOVE_FILTER_RESET_DT,
                                Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE,
                                alphas[i], Config<T>::MOVE_FILTER_POSITION_VARIANCE_1_1,
                                Config<T>::MOVE_FILTER_ANGLE_VARIANCE_1_1));
                }

                if (ORDER_P == 2 && ORDER_A == 1)
                {
                        res.push_back(std::make_unique<move::Move21<T>>(
                                name(alphas[i]), color::RGB8(0, 160 - 40 * i, 50), Config<T>::MOVE_FILTER_RESET_DT,
                                Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE,
                                alphas[i], Config<T>::MOVE_FILTER_POSITION_VARIANCE_2_1,
                                Config<T>::MOVE_FILTER_ANGLE_VARIANCE_2_1));
                }
        }

        return res;
}

template <std::size_t N, typename T>
void write_result(
        const std::string_view annotation,
        const std::vector<Measurements<N, T>>& measurements,
        const std::vector<position::PositionVariance<N, T>>& position_variance,
        const std::vector<const std::vector<std::unique_ptr<position::Position<N, T>>>*>& positions,
        const std::vector<const std::vector<std::unique_ptr<process::Process<T>>>*>& processes,
        const std::vector<const std::vector<std::unique_ptr<move::Move<T>>>*>& moves)
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
                                log_consistency_string(*p1);
                        }
                }
        };

        log(positions);

        log(processes);

        log(moves);
}

template <std::size_t N, typename T>
std::optional<Vector<N, T>> compute_variance(const std::vector<position::PositionVariance<N, T>>& positions)
{
        if (positions.size() == 1)
        {
                return positions.front().last_position_variance();
        }

        Vector<N, T> sum(0);
        std::size_t count = 0;
        for (const position::PositionVariance<N, T>& position : positions)
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

        std::vector<position::PositionVariance<2, T>> position_variance = create_position_variance<2, T>();
        std::vector<std::unique_ptr<position::Position<2, T>>> positions_0 = create_positions<2, T, 0>();
        std::vector<std::unique_ptr<position::Position<2, T>>> positions_1 = create_positions<2, T, 1>();
        std::vector<std::unique_ptr<position::Position<2, T>>> positions_2 = create_positions<2, T, 2>();
        std::vector<std::unique_ptr<process::Process<T>>> processes = create_processes<T>();
        std::vector<std::unique_ptr<move::Move<T>>> moves_1_0 = create_moves<T, 1, 0>();
        std::vector<std::unique_ptr<move::Move<T>>> moves_1_1 = create_moves<T, 1, 1>();
        std::vector<std::unique_ptr<move::Move<T>>> moves_2_1 = create_moves<T, 2, 1>();

        position::PositionEstimation<T> position_estimation(
                Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_TIME_DIFFERENCE,
                static_cast<const position::Position2<2, T>*>(positions_2.front().get()));

        const auto update = [&](const Measurements<2, T>& measurement)
        {
                for (auto& p : positions_2)
                {
                        p->update_position(measurement);
                }

                position_estimation.update(measurement);

                for (auto& p : positions_1)
                {
                        p->update_position(measurement);
                }

                for (auto& p : processes)
                {
                        p->update(measurement, std::as_const(position_estimation));
                }

                for (auto& m : moves_1_0)
                {
                        m->update(measurement, std::as_const(position_estimation));
                }

                for (auto& m : moves_1_1)
                {
                        m->update(measurement, std::as_const(position_estimation));
                }

                for (auto& m : moves_2_1)
                {
                        m->update(measurement, std::as_const(position_estimation));
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
                track.annotation(), measurements, position_variance, {&positions_0, &positions_1, &positions_2},
                {&processes}, {&moves_1_0, &moves_1_1, &moves_2_1});
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
