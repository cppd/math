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
        static constexpr T TRACK_SPEED_MIN = 3;
        static constexpr T TRACK_SPEED_MAX = 30;
        static constexpr T TRACK_SPEED_VARIANCE = square(0.1);
        static constexpr T TRACK_DIRECTION_BIAS_DRIFT = degrees_to_radians(360.0);
        static constexpr T TRACK_DIRECTION_ANGLE = degrees_to_radians(30.0);

        static constexpr T MEASUREMENT_DT = 0.1L;
        static constexpr unsigned MEASUREMENT_DT_COUNT_ACCELERATION = 1;
        static constexpr unsigned MEASUREMENT_DT_COUNT_DIRECTION = 1;
        static constexpr unsigned MEASUREMENT_DT_COUNT_POSITION = 10;
        static constexpr unsigned MEASUREMENT_DT_COUNT_SPEED = 10;

        static constexpr T MEASUREMENT_VARIANCE_ACCELERATION = square(1.0);
        static constexpr T MEASUREMENT_VARIANCE_DIRECTION = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_VARIANCE_POSITION = square(20.0);
        static constexpr T MEASUREMENT_VARIANCE_SPEED = square(0.2);

        static constexpr T POSITION_FILTER_VARIANCE = square(0.5);
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(10.0));
        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(0.1);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr std::array PROCESS_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});

        static constexpr T DATA_CONNECT_INTERVAL = 2;
};

template <std::size_t N, typename T>
std::string make_annotation(const std::vector<ProcessMeasurement<N, T>>& measurements)
{
        bool position = false;
        bool speed = false;
        bool direction = false;
        bool acceleration = false;

        for (const ProcessMeasurement<N, T>& m : measurements)
        {
                position = position || m.position.has_value();
                speed = speed || m.speed.has_value();
                direction = direction || m.direction.has_value();
                acceleration = acceleration || m.acceleration.has_value();
        }

        if (!position)
        {
                error("No position measurements");
        }

        constexpr std::string_view DEGREE = "&#x00b0;";
        constexpr std::string_view SIGMA = "&#x03c3;";
        std::ostringstream oss;

        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / (Config<T>::MEASUREMENT_DT * Config<T>::MEASUREMENT_DT_COUNT_POSITION) << " Hz";
        if (speed)
        {
                oss << "<br>";
                oss << "speed: " << 1 / (Config<T>::MEASUREMENT_DT * Config<T>::MEASUREMENT_DT_COUNT_SPEED) << " Hz";
        }
        if (direction)
        {
                oss << "<br>";
                oss << "direction: " << 1 / (Config<T>::MEASUREMENT_DT * Config<T>::MEASUREMENT_DT_COUNT_DIRECTION)
                    << " Hz";
        }
        if (acceleration)
        {
                oss << "<br>";
                oss << "acceleration: "
                    << 1 / (Config<T>::MEASUREMENT_DT * Config<T>::MEASUREMENT_DT_COUNT_ACCELERATION) << " Hz";
        }
        if (direction || acceleration)
        {
                oss << "<br>";
                oss << "<br>";
                oss << "<b>bias</b>";
                oss << "<br>";
                oss << "direction drift: " << radians_to_degrees(Config<T>::TRACK_DIRECTION_BIAS_DRIFT) << " " << DEGREE
                    << "/h";
                oss << "<br>";
                oss << "direction angle: " << radians_to_degrees(Config<T>::TRACK_DIRECTION_ANGLE) << DEGREE;
        }
        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "position: " << std::sqrt(Config<T>::MEASUREMENT_VARIANCE_POSITION) << " m";
        if (speed)
        {
                oss << "<br>";
                oss << "speed: " << std::sqrt(Config<T>::MEASUREMENT_VARIANCE_SPEED) << " m/s";
        }
        if (direction)
        {
                oss << "<br>";
                oss << "direction: " << radians_to_degrees(std::sqrt(Config<T>::MEASUREMENT_VARIANCE_DIRECTION))
                    << DEGREE;
        }
        if (acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << std::sqrt(Config<T>::MEASUREMENT_VARIANCE_ACCELERATION)
                    << " m/s<sup>2</sup>";
        }

        return oss.str();
}

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

        view::write_to_file(make_annotation(track.measurements), track, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <std::size_t N, typename T>
Track<N, T> track()
{
        return generate_track<N, T>(
                {.track_speed_min = Config<T>::TRACK_SPEED_MIN,
                 .track_speed_max = Config<T>::TRACK_SPEED_MAX,
                 .track_speed_variance = Config<T>::TRACK_SPEED_VARIANCE,
                 .track_direction_bias_drift = Config<T>::TRACK_DIRECTION_BIAS_DRIFT,
                 .track_direction_angle = Config<T>::TRACK_DIRECTION_ANGLE,
                 .measurement_dt = Config<T>::MEASUREMENT_DT,
                 .measurement_dt_count_acceleration = Config<T>::MEASUREMENT_DT_COUNT_ACCELERATION,
                 .measurement_dt_count_direction = Config<T>::MEASUREMENT_DT_COUNT_DIRECTION,
                 .measurement_dt_count_position = Config<T>::MEASUREMENT_DT_COUNT_POSITION,
                 .measurement_dt_count_speed = Config<T>::MEASUREMENT_DT_COUNT_SPEED,
                 .measurement_variance_acceleration = Config<T>::MEASUREMENT_VARIANCE_ACCELERATION,
                 .measurement_variance_direction = Config<T>::MEASUREMENT_VARIANCE_DIRECTION,
                 .measurement_variance_position = Config<T>::MEASUREMENT_VARIANCE_POSITION,
                 .measurement_variance_speed = Config<T>::MEASUREMENT_VARIANCE_SPEED});
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
std::tuple<typename std::vector<ProcessMeasurement<2, T>>::const_iterator, T, Vector<2, T>, Vector<2, T>>
        estimate_direction(
                const Track<2, T>& track,
                typename std::vector<ProcessMeasurement<2, T>>::const_iterator iter,
                std::vector<Position<T>>* const positions)
{
        static constexpr T DIRECTION_TIME_DIFFERENCE = 1;

        if (iter == track.measurements.cend())
        {
                error("No measurements to estimate direction");
        }

        std::optional<typename std::vector<ProcessMeasurement<2, T>>::const_iterator> direction_iter;
        if (iter->direction)
        {
                direction_iter = iter;
        }

        const Position<T>* angle_position = nullptr;
        T angle_p = Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE;

        for (; iter != track.measurements.cend(); ++iter)
        {
                if (iter->direction)
                {
                        direction_iter = iter;
                }

                if (!iter->position)
                {
                        continue;
                }

                const bool direction =
                        direction_iter && (iter->time - (*direction_iter)->time <= DIRECTION_TIME_DIFFERENCE)
                        && (*direction_iter)->direction;

                for (Position<T>& position : *positions)
                {
                        position.update(
                                *iter, Config<T>::MEASUREMENT_VARIANCE_POSITION,
                                track.points[iter->simulator_point_index]);

                        LOG(to_string(iter->time) + "; " + position.name()
                            + "; angle p = " + to_string(radians_to_degrees(std::sqrt(position.angle_p()))));

                        const T position_angle_p = position.angle_p();
                        if (direction && (position_angle_p < angle_p))
                        {
                                angle_position = &position;
                                angle_p = position_angle_p;
                        }
                }

                if (angle_position)
                {
                        break;
                }
        }

        if (iter == track.measurements.cend())
        {
                LOG("Failed to estimate direction");
                return {iter, 0, Vector<2, T>(0), Vector<2, T>(0)};
        }

        ASSERT(angle_position);
        LOG("angle from " + angle_position->name());

        const T position_filter_angle = angle_position->angle();
        const T position_filter_angle_p = angle_position->angle_p();
        const T measurement_angle = *(*direction_iter)->direction;
        const T angle_difference = normalize_angle(measurement_angle - position_filter_angle);

        LOG("estimation: angle = " + to_string(radians_to_degrees(position_filter_angle)) + "; "
            + "angle stddev = " + to_string(radians_to_degrees(std::sqrt(position_filter_angle_p))) + "\n"
            + "measurement: angle = " + to_string(radians_to_degrees(measurement_angle)) + "\n"
            + "angle difference = " + to_string(radians_to_degrees(angle_difference)));

        return {iter, angle_difference, angle_position->position(), angle_position->velocity()};
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
                create_positions(*first_position_iter->position, Config<T>::MEASUREMENT_VARIANCE_POSITION);

        const auto [first_process_iter, init_angle, init_position, init_velocity] =
                estimate_direction(track, std::next(first_position_iter), &positions);

        std::vector<Process<T>> processes;
        if (first_process_iter != track.measurements.cend())
        {
                processes = create_processes(
                        init_position, init_velocity, init_angle, Config<T>::MEASUREMENT_VARIANCE_POSITION);
        }

        auto iter =
                first_process_iter != track.measurements.cend() ? std::next(first_process_iter) : first_process_iter;
        for (; iter != track.measurements.cend(); ++iter)
        {
                const T pv = Config<T>::MEASUREMENT_VARIANCE_POSITION;
                const T sv = Config<T>::MEASUREMENT_VARIANCE_SPEED;
                const T dv = Config<T>::MEASUREMENT_VARIANCE_DIRECTION;
                const T av = Config<T>::MEASUREMENT_VARIANCE_ACCELERATION;

                const auto& point = track.points[iter->simulator_point_index];

                if (iter->position)
                {
                        for (auto& p : positions)
                        {
                                p.update(*iter, pv, point);
                        }
                }

                for (auto& p : processes)
                {
                        p.update(*iter, pv, sv, dv, av, point);
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
