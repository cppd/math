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

#include "position.h"
#include "position_filter_lkf.h"
#include "process.h"
#include "process_filter_ekf.h"
#include "process_filter_ukf.h"
#include "simulator.h"
#include "utility.h"

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
        static constexpr T POSITION_FILTER_VARIANCE = square(0.5);
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(10.0));
        static constexpr T POSITION_FILTER_DIRECTION_TIME_DIFFERENCE = 1;
        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(0.1);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr std::array PROCESS_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});

        static constexpr T DATA_CONNECT_INTERVAL = 2;
};

template <typename T>
void write_to_file(
        const Track<2, T>& track,
        const std::vector<Position<T>>& positions,
        const std::vector<Process<T>>& processes)
{
        std::vector<view::Filter<2, T>> filters;
        filters.reserve(positions.size() + processes.size());

        for (const Position<T>& position : positions)
        {
                filters.push_back(
                        {.name = position.name(),
                         .color = position.color(),
                         .speed = position.speeds(),
                         .position = position.positions()});
        }

        for (const Process<T>& process : processes)
        {
                filters.push_back(
                        {.name = process.name(),
                         .color = process.color(),
                         .speed = process.speeds(),
                         .position = process.positions()});
        }

        view::write_to_file(track, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <typename T>
typename std::vector<ProcessMeasurement<2, T>>::const_iterator find_position(const Track<2, T>& track)
{
        auto iter = track.measurements.cbegin();
        while (iter != track.measurements.cend() && !iter->position)
        {
                ++iter;
        }
        if (iter == track.measurements.cend())
        {
                error("No position measurement found");
        }
        return iter;
}

template <typename T>
struct PositionEstimate final
{
        T angle;
        Vector<2, T> position;
        Vector<2, T> velocity;
};

template <typename T>
class PositionEstimation final
{
        const std::vector<Position<T>>* const positions_;
        std::optional<T> last_direction_;
        std::optional<T> last_direction_time_;
        std::optional<std::size_t> angle_position_index_;
        bool direction_ = false;

public:
        explicit PositionEstimation(const std::vector<Position<T>>* const positions)
                : positions_(positions)
        {
                ASSERT(positions_);
                last_direction_time_ = std::nullopt;
        }

        void update(const ProcessMeasurement<2, T>& m)
        {
                if (m.direction)
                {
                        last_direction_ = *m.direction;
                        last_direction_time_ = m.time;
                }

                direction_ =
                        last_direction_time_
                        && (m.time - *last_direction_time_ <= Config<T>::POSITION_FILTER_DIRECTION_TIME_DIFFERENCE);

                angle_position_index_.reset();

                if (!direction_)
                {
                        return;
                }

                T angle_p = Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE;
                for (std::size_t i = 0; i < positions_->size(); ++i)
                {
                        const Position<T>& position = (*positions_)[i];

                        LOG(to_string(m.time) + "; " + position.name()
                            + "; angle p = " + to_string(radians_to_degrees(std::sqrt(position.angle_p()))));

                        const T position_angle_p = position.angle_p();
                        if (position_angle_p < angle_p)
                        {
                                angle_position_index_ = i;
                                angle_p = position_angle_p;
                        }
                }
        }

        [[nodiscard]] bool has_value() const
        {
                ASSERT(!direction_ || last_direction_);
                return direction_ && angle_position_index_;
        }

        [[nodiscard]] PositionEstimate<T> value() const
        {
                if (!has_value())
                {
                        error("Estimation doesn't have value");
                }
                const Position<T>& position = (*positions_)[*angle_position_index_];
                return {.angle = normalize_angle(*last_direction_ - position.angle()),
                        .position = position.position(),
                        .velocity = position.velocity()};
        }

        [[nodiscard]] std::string description() const
        {
                if (!has_value())
                {
                        error("Estimation doesn't have value");
                }

                const Position<T>& position = (*positions_)[*angle_position_index_];
                const T filter_angle = position.angle();

                std::string res;
                res += "estimation:";
                res += "\nfilter = " + position.name();
                res += "\nangle = " + to_string(radians_to_degrees(filter_angle));
                res += "\nangle stddev = " + to_string(radians_to_degrees(std::sqrt(position.angle_p())));
                res += "\nmeasurement: angle = " + to_string(radians_to_degrees(*last_direction_));
                res += "\nangle difference = "
                       + to_string(radians_to_degrees(normalize_angle(*last_direction_ - filter_angle)));
                return res;
        }
};

template <typename T>
std::tuple<typename std::vector<ProcessMeasurement<2, T>>::const_iterator, PositionEstimate<T>> estimate_direction(
        const Track<2, T>& track,
        typename std::vector<ProcessMeasurement<2, T>>::const_iterator iter,
        std::vector<Position<T>>* const positions)
{
        if (iter == track.measurements.cend())
        {
                error("No measurements to estimate direction");
        }

        PositionEstimation estimation(positions);

        for (; iter != track.measurements.cend(); ++iter)
        {
                if (!iter->position)
                {
                        continue;
                }

                for (Position<T>& position : *positions)
                {
                        position.update(*iter, track.points[iter->simulator_point_index]);
                }

                estimation.update(*iter);

                if (estimation.has_value())
                {
                        break;
                }
        }

        if (iter == track.measurements.cend())
        {
                LOG("Failed to estimate direction");
                return {iter, {}};
        }

        ASSERT(estimation.has_value());

        LOG(estimation.description());

        return {iter, estimation.value()};
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

template <typename T>
std::vector<Position<T>> create_positions(const Vector<2, T>& init_position, const T init_position_variance)
{
        const PositionFilterInit<T> init{.position = init_position, .position_variance = init_position_variance};

        std::vector<Position<T>> res;

        const int precision = compute_precision(Config<T>::POSITION_FILTER_THETAS);

        const auto name = [&](const T theta)
        {
                const auto* const letter_theta = reinterpret_cast<const char*>(u8"\u03b8");
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "LKF (" << letter_theta << " " << theta << ")";
                return oss.str();
        };

        const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                ASSERT(i <= 4);
                res.emplace_back(
                        name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0),
                        create_position_filter_lkf(init, thetas[i], Config<T>::POSITION_FILTER_VARIANCE));
        }

        return res;
}

template <typename T>
std::vector<Process<T>> create_processes(
        const Vector<2, T>& init_position,
        const Vector<2, T>& init_velocity,
        const T init_angle,
        const T init_position_variance)
{
        const ProcessFilterInit<T> init{
                .position = init_position,
                .velocity = init_velocity,
                .angle = init_angle,
                .position_variance = init_position_variance};

        const T process_pv = Config<T>::PROCESS_FILTER_POSITION_VARIANCE;
        const T process_av = Config<T>::PROCESS_FILTER_ANGLE_VARIANCE;
        const T process_arv = Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE;

        std::vector<Process<T>> res;

        res.emplace_back(
                "EKF", color::RGB8(0, 200, 0), create_process_filter_ekf<T>(init, process_pv, process_av, process_arv));

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
                        name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0),
                        create_process_filter_ukf(init, alphas[i], process_pv, process_av, process_arv));
        }

        return res;
}

template <typename T>
void test_impl(const Track<2, T>& track)
{
        ASSERT(!track.measurements.empty());

        const auto first_position_iter = find_position(track);
        ASSERT(first_position_iter != track.measurements.cend() && first_position_iter->position);

        std::vector<Position<T>> positions =
                create_positions(*first_position_iter->position, first_position_iter->position_variance);

        const auto [first_process_iter, estimate] =
                estimate_direction(track, std::next(first_position_iter), &positions);

        std::vector<Process<T>> processes;
        if (first_process_iter != track.measurements.cend())
        {
                processes = create_processes(
                        estimate.position, estimate.velocity, estimate.angle, first_process_iter->position_variance);
        }

        auto iter =
                first_process_iter != track.measurements.cend() ? std::next(first_process_iter) : first_process_iter;
        for (; iter != track.measurements.cend(); ++iter)
        {
                const auto& point = track.points[iter->simulator_point_index];

                if (iter->position)
                {
                        for (auto& p : positions)
                        {
                                p.update(*iter, point);
                        }
                }

                for (auto& p : processes)
                {
                        p.update(*iter, point);
                }

                if (iter->position)
                {
                        for (auto& p : processes)
                        {
                                LOG(to_string(iter->time) + "; " + p.angle_string(point));
                        }
                }
        }

        write_to_file(track, positions, processes);

        for (const auto& p : positions)
        {
                LOG(p.nees_string());
        }

        for (const auto& p : processes)
        {
                LOG(p.nees_string());
        }
}

template <typename T>
void test_impl()
{
        test_impl(generate_track<2, T>());
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
