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
        static constexpr T DT = 0.1;
        static constexpr std::size_t POSITION_INTERVAL = 10;

        static constexpr T TRACK_VELOCITY_MIN = 3;
        static constexpr T TRACK_VELOCITY_MAX = 30;
        static constexpr T TRACK_VELOCITY_VARIANCE = square(0.1);

        static constexpr T DIRECTION_BIAS_DRIFT = degrees_to_radians(360.0);
        static constexpr T DIRECTION_ANGLE = degrees_to_radians(30.0);

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
        oss << "position: " << 1 / (Config<T>::POSITION_INTERVAL * Config<T>::DT) << " Hz";
        oss << "<br>";
        oss << "speed: " << 1 / (Config<T>::POSITION_INTERVAL * Config<T>::DT) << " Hz";
        oss << "<br>";
        oss << "direction: " << 1 / Config<T>::DT << " Hz";
        oss << "<br>";
        oss << "acceleration: " << 1 / Config<T>::DT << " Hz";
        oss << "<br>";
        oss << "<br>";
        oss << "<b>bias</b>";
        oss << "<br>";
        oss << "direction drift: " << radians_to_degrees(Config<T>::DIRECTION_BIAS_DRIFT) << " " << DEGREE << "/h";
        oss << "<br>";
        oss << "direction angle: " << radians_to_degrees(Config<T>::DIRECTION_ANGLE) << DEGREE;
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
        filters.reserve(processes.size());

        for (const Process<T>& fd : processes)
        {
                filters.push_back(
                        {.name = fd.name(), .color = fd.color(), .speed = fd.speed(), .position = fd.position()});
        }

        view::write_to_file(
                make_annotation<T>(), track, Config<T>::POSITION_INTERVAL, position.speed(), position.positions(),
                filters);
}

template <std::size_t N, typename T>
Track<N, T> generate_track()
{
        constexpr std::size_t COUNT = 10000;

        const TrackMeasurementVariance<T> measurement_variance{
                .direction = Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                .acceleration = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                .position = Config<T>::MEASUREMENT_POSITION_VARIANCE,
                .position_speed = Config<T>::MEASUREMENT_SPEED_VARIANCE};

        Track res = generate_track<N, T>(
                COUNT, Config<T>::DT, Config<T>::TRACK_VELOCITY_MIN, Config<T>::TRACK_VELOCITY_MAX,
                Config<T>::TRACK_VELOCITY_VARIANCE, Config<T>::DIRECTION_BIAS_DRIFT, Config<T>::DIRECTION_ANGLE,
                measurement_variance, Config<T>::POSITION_INTERVAL);

        std::vector<PositionMeasurement<N, T>> measurements;
        measurements.reserve(res.position_measurements.size());
        std::optional<std::size_t> last_index;

        for (PositionMeasurement<N, T> m : res.position_measurements)
        {
                ASSERT(m.index >= 0 && m.index < COUNT);
                ASSERT(!last_index || *last_index < m.index);
                last_index = m.index;

                const auto n = std::llround(m.index / T{300});
                if ((n > 3) && ((n % 5) == 0))
                {
                        continue;
                }
                if (std::llround(m.index / T{100}) % 5 == 0)
                {
                        m.speed.reset();
                }

                measurements.push_back(m);
        }

        res.position_measurements = measurements;

        return res;
}

template <typename T>
typename std::vector<PositionMeasurement<2, T>>::const_iterator find_direction(
        const Track<2, T>& track,
        Position<T>* const position)
{
        for (auto iter = track.position_measurements.cbegin(); iter != track.position_measurements.cend(); ++iter)
        {
                position->update(*iter, Config<T>::MEASUREMENT_POSITION_VARIANCE, track.points[iter->index]);
                if (position->filter().angle_p() < Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE)
                {
                        return iter;
                }
        }
        error("Failed to estimate direction");
}

template <typename T>
typename std::vector<ProcessMeasurement<2, T>>::const_iterator move_process(
        const Track<2, T>& track,
        const std::size_t index)
{
        auto process_iter = track.process_measurements.cbegin();
        while (process_iter != track.process_measurements.cend() && process_iter->index <= index)
        {
                ++process_iter;
        }
        if (process_iter == track.process_measurements.cend())
        {
                error("Process position is not found");
        }
        return process_iter;
}

template <typename T>
void test_impl(const Track<2, T>& track)
{
        ASSERT(!track.position_measurements.empty() && track.position_measurements[0].index == 0);

        Position<T> position(
                PositionFilter(
                        Config<T>::POSITION_FILTER_VARIANCE,
                        {track.position_measurements[0].position[0], 1.0, -1.0,
                         track.position_measurements[0].position[1], -2.0, 0.5},
                        make_diagonal_matrix<6, T>(
                                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10)})),
                Config<T>::DT, Config<T>::POSITION_INTERVAL);

        auto position_iter = find_direction(track, &position);

        const T position_filter_angle = position.filter().angle();
        const T position_filter_angle_p = position.filter().angle_p();
        const T measurement_angle = track.process_measurements[position_iter->index].direction;
        const T angle_difference = normalize_angle(measurement_angle - position_filter_angle);

        LOG("estimation: angle = " + to_string(radians_to_degrees(position_filter_angle)) + "; "
            + "angle stddev = " + to_string(radians_to_degrees(std::sqrt(position_filter_angle_p))) + "\n"
            + "measurement: angle = " + to_string(radians_to_degrees(measurement_angle)) + "\n"
            + "angle difference = " + to_string(radians_to_degrees(angle_difference)));

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
                "EKF", 190,
                create_process_filter_ekf<T>(
                        Config<T>::PROCESS_FILTER_POSITION_VARIANCE, Config<T>::PROCESS_FILTER_ANGLE_VARIANCE,
                        Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE, init_x, init_p),
                Config<T>::DT);

        processes.emplace_back(
                "UKF", 140,
                create_process_filter_ukf(
                        Config<T>::PROCESS_FILTER_POSITION_VARIANCE, Config<T>::PROCESS_FILTER_ANGLE_VARIANCE,
                        Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE, init_x, init_p),
                Config<T>::DT);

        auto process_iter = move_process(track, position_iter->index);
        ++position_iter;

        for (; process_iter != track.process_measurements.cend(); ++process_iter)
        {
                ASSERT(position_iter == track.position_measurements.cend()
                       || position_iter->index >= process_iter->index);

                for (auto& p : processes)
                {
                        p.predict(process_iter->index);
                }

                if (position_iter != track.position_measurements.cend() && position_iter->index == process_iter->index)
                {
                        position.update(
                                *position_iter, Config<T>::MEASUREMENT_POSITION_VARIANCE,
                                track.points[position_iter->index]);

                        for (auto& p : processes)
                        {
                                p.update(
                                        *position_iter, *process_iter, Config<T>::MEASUREMENT_POSITION_VARIANCE,
                                        Config<T>::MEASUREMENT_SPEED_VARIANCE,
                                        Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                                        Config<T>::MEASUREMENT_ACCELERATION_VARIANCE, process_iter->index,
                                        track.points[process_iter->index]);

                                LOG(to_string(process_iter->index) + "; "
                                    + p.angle_string(track.points[process_iter->index]));
                        }

                        ++position_iter;
                }
                else
                {
                        for (auto& p : processes)
                        {
                                p.update_acceleration(
                                        *process_iter, Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                                        process_iter->index, track.points[process_iter->index]);
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
        const Track track = generate_track<2, T>();
        test_impl(track);
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
