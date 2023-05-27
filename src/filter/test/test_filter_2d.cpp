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
#include "position_filter.h"
#include "process.h"
#include "process_filter_ekf.h"
#include "process_filter_ukf.h"
#include "simulator.h"
#include "utility.h"

#include "view/write.h"

#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
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
        static constexpr T DT = T{1} / 10;
        static constexpr T POSITION_DT = 1;
        static constexpr T SPEED_DT = 1;

        static constexpr T DATA_CONNECT_INTERVAL = 1.5 * POSITION_DT;

        static constexpr T TRACK_SPEED_MIN = 3;
        static constexpr T TRACK_SPEED_MAX = 30;
        static constexpr T TRACK_SPEED_VARIANCE = square(0.1);
        static constexpr T TRACK_DIRECTION_BIAS_DRIFT = degrees_to_radians(360.0);
        static constexpr T TRACK_DIRECTION_ANGLE = degrees_to_radians(30.0);

        static constexpr T MEASUREMENT_DIRECTION_VARIANCE = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_ACCELERATION_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_POSITION_VARIANCE = square(20.0);
        static constexpr T MEASUREMENT_SPEED_VARIANCE = square(0.2);

        static constexpr T POSITION_FILTER_VARIANCE = square(0.5);
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(10.0));

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(0.1);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
};

template <typename T>
std::string make_annotation()
{
        constexpr std::string_view DEGREE = "&#x00b0;";
        constexpr std::string_view SIGMA = "&#x03c3;";
        std::ostringstream oss;
        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / Config<T>::POSITION_DT << " Hz";
        oss << "<br>";
        oss << "speed: " << 1 / Config<T>::SPEED_DT << " Hz";
        oss << "<br>";
        oss << "direction: " << 1 / Config<T>::DT << " Hz";
        oss << "<br>";
        oss << "acceleration: " << 1 / Config<T>::DT << " Hz";
        oss << "<br>";
        oss << "<br>";
        oss << "<b>bias</b>";
        oss << "<br>";
        oss << "direction drift: " << radians_to_degrees(Config<T>::TRACK_DIRECTION_BIAS_DRIFT) << " " << DEGREE
            << "/h";
        oss << "<br>";
        oss << "direction angle: " << radians_to_degrees(Config<T>::TRACK_DIRECTION_ANGLE) << DEGREE;
        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "position: " << std::sqrt(Config<T>::MEASUREMENT_POSITION_VARIANCE) << " m";
        oss << "<br>";
        oss << "speed: " << std::sqrt(Config<T>::MEASUREMENT_SPEED_VARIANCE) << " m/s";
        oss << "<br>";
        oss << "direction: " << radians_to_degrees(std::sqrt(Config<T>::MEASUREMENT_DIRECTION_VARIANCE)) << DEGREE;
        oss << "<br>";
        oss << "acceleration: " << std::sqrt(Config<T>::MEASUREMENT_ACCELERATION_VARIANCE) << " m/s<sup>2</sup>";
        return oss.str();
}

template <typename T>
void write_to_file(const Track<2, T>& track, const Position<T>& position, const std::vector<Process<T>>& processes)
{
        std::vector<view::Filter<2, T>> filters;
        filters.reserve(1 + processes.size());

        filters.push_back(
                {.name = position.name(),
                 .color = position.color(),
                 .speed = position.speed(),
                 .position = position.position()});

        for (const Process<T>& process : processes)
        {
                filters.push_back(
                        {.name = process.name(),
                         .color = process.color(),
                         .speed = process.speed(),
                         .position = process.position()});
        }

        view::write_to_file(make_annotation<T>(), track, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <std::size_t N, typename T>
void process_track(std::vector<ProcessMeasurement<N, T>>* const measurements)
{
        std::optional<T> last_time;
        std::optional<T> last_position_time;
        std::optional<T> last_speed_time;

        for (ProcessMeasurement<N, T>& m : *measurements)
        {
                if (last_time && !(*last_time < m.time))
                {
                        ASSERT(false);
                        continue;
                }
                last_time = m.time;

                if (last_position_time && !(*last_position_time + Config<T>::POSITION_DT <= m.time))
                {
                        m.position.reset();
                }
                else
                {
                        last_position_time = m.time;
                }

                if (last_speed_time && !(*last_speed_time + Config<T>::SPEED_DT <= m.time))
                {
                        m.speed.reset();
                }
                else
                {
                        last_speed_time = m.time;
                }
        }
}

template <std::size_t N, typename T>
Track<N, T> track()
{
        constexpr std::size_t COUNT = 10000;

        Track track = generate_track<N, T>(
                COUNT,
                {.dt = Config<T>::DT,
                 .speed_min = Config<T>::TRACK_SPEED_MIN,
                 .speed_max = Config<T>::TRACK_SPEED_MAX,
                 .speed_variance = Config<T>::TRACK_SPEED_VARIANCE,
                 .direction_bias_drift = Config<T>::TRACK_DIRECTION_BIAS_DRIFT,
                 .direction_angle = Config<T>::TRACK_DIRECTION_ANGLE},
                {.direction = Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                 .acceleration = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                 .position = Config<T>::MEASUREMENT_POSITION_VARIANCE,
                 .speed = Config<T>::MEASUREMENT_SPEED_VARIANCE});

        process_track(&track.measurements);

        for (ProcessMeasurement<N, T>& m : track.measurements)
        {
                ASSERT(m.simulator_point_index >= 0 && m.simulator_point_index < track.points.size());

                const auto n = std::llround(m.time / 30);
                if ((n > 3) && ((n % 5) == 0))
                {
                        m.position.reset();
                        m.speed.reset();
                }

                if ((std::llround(m.time / 10) % 8) == 0)
                {
                        m.speed.reset();
                }
        }

        return track;
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
std::tuple<typename std::vector<ProcessMeasurement<2, T>>::const_iterator, T> estimate_direction(
        const Track<2, T>& track,
        typename std::vector<ProcessMeasurement<2, T>>::const_iterator iter,
        Position<T>* const position)
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

                position->update(
                        *iter, Config<T>::MEASUREMENT_POSITION_VARIANCE, track.points[iter->simulator_point_index]);

                if (!direction_iter || !(iter->time - (*direction_iter)->time <= DIRECTION_TIME_DIFFERENCE))
                {
                        continue;
                }
                if (!(*direction_iter)->direction)
                {
                        continue;
                }
                if (position->filter().angle_p() < Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE)
                {
                        break;
                }
        }

        if (iter == track.measurements.cend())
        {
                error("Failed to estimate direction");
        }

        const T position_filter_angle = position->filter().angle();
        const T position_filter_angle_p = position->filter().angle_p();
        const T measurement_angle = *(*direction_iter)->direction;
        const T angle_difference = normalize_angle(measurement_angle - position_filter_angle);

        LOG("estimation: angle = " + to_string(radians_to_degrees(position_filter_angle)) + "; "
            + "angle stddev = " + to_string(radians_to_degrees(std::sqrt(position_filter_angle_p))) + "\n"
            + "measurement: angle = " + to_string(radians_to_degrees(measurement_angle)) + "\n"
            + "angle difference = " + to_string(radians_to_degrees(angle_difference)));

        return {iter, angle_difference};
}

template <typename T>
void test_impl(const Track<2, T>& track)
{
        ASSERT(!track.measurements.empty());

        auto iter = find_position(track);
        ASSERT(iter != track.measurements.cend() && iter->position);

        Position<T> position(
                "LKF", color::RGB8(160, 0, 0),
                PositionFilter(
                        Config<T>::POSITION_FILTER_VARIANCE,
                        {(*iter->position)[0], 1.0, -1.0, (*iter->position)[1], -2.0, 0.5},
                        make_diagonal_matrix<6, T>(
                                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10)})));

        ++iter;

        T angle_difference;
        std::tie(iter, angle_difference) = estimate_direction(track, iter, &position);
        ASSERT(iter != track.measurements.cend());

        const Vector<9, T> init_x = [&]()
        {
                const Vector<2, T> p = position.filter().position();
                const Vector<2, T> v = position.filter().velocity();
                const Vector<2, T> a(0);
                return Vector<9, T>(p[0], v[0], a[0], p[1], v[1], a[1], angle_difference, 0, 0);
        }();

        const Matrix<9, 9, T> init_p = [&]()
        {
                const T p = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                const T v = square(30.0);
                const T a = square(10.0);
                const T angle = square(degrees_to_radians(40.0));
                const T angle_speed = square(degrees_to_radians(0.1));
                const T angle_r = square(degrees_to_radians(40.0));
                return make_diagonal_matrix<9, T>({p, v, a, p, v, a, angle, angle_speed, angle_r});
        }();

        std::vector<Process<T>> processes;

        processes.emplace_back(
                "EKF", color::RGB8(0, 190, 0),
                create_process_filter_ekf<T>(
                        Config<T>::PROCESS_FILTER_POSITION_VARIANCE, Config<T>::PROCESS_FILTER_ANGLE_VARIANCE,
                        Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE, init_x, init_p));

        processes.emplace_back(
                "UKF", color::RGB8(0, 140, 0),
                create_process_filter_ukf(
                        Config<T>::PROCESS_FILTER_POSITION_VARIANCE, Config<T>::PROCESS_FILTER_ANGLE_VARIANCE,
                        Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE, init_x, init_p));

        ++iter;

        for (; iter != track.measurements.cend(); ++iter)
        {
                const T pv = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                const T sv = Config<T>::MEASUREMENT_SPEED_VARIANCE;
                const T dv = Config<T>::MEASUREMENT_DIRECTION_VARIANCE;
                const T av = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE;

                const auto& point = track.points[iter->simulator_point_index];

                if (iter->position)
                {
                        position.update(*iter, pv, point);
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

        write_to_file(track, position, processes);

        LOG(position.nees_string());
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
