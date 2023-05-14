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

#include "position_filter.h"
#include "process_filter_ekf.h"
#include "simulator.h"
#include "utility.h"

#include "view/write.h"

#include "../nees.h"

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <cmath>
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

        static constexpr T TRACK_VELOCITY_MEAN = 20;
        static constexpr T TRACK_VELOCITY_VARIANCE = square(0.1);

        static constexpr T DIRECTION_BIAS_DRIFT = degrees_to_radians(360.0);
        static constexpr T DIRECTION_ANGLE = degrees_to_radians(10.0);

        static constexpr T MEASUREMENT_DIRECTION_VARIANCE = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_ACCELERATION_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_POSITION_VARIANCE = square(20.0);
        static constexpr T MEASUREMENT_SPEED_VARIANCE = square(0.2);

        static constexpr T POSITION_FILTER_VARIANCE = square(0.2);

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(0.2);
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

template <std::size_t N, typename T>
Track<N, T> generate_track()
{
        constexpr std::size_t COUNT = 6000;

        const TrackMeasurementVariance<T> measurement_variance{
                .direction = Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                .acceleration = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                .position = Config<T>::MEASUREMENT_POSITION_VARIANCE,
                .position_speed = Config<T>::MEASUREMENT_SPEED_VARIANCE};

        Track res = generate_track<N, T>(
                COUNT, Config<T>::DT, Config<T>::TRACK_VELOCITY_MEAN, Config<T>::TRACK_VELOCITY_VARIANCE,
                Config<T>::DIRECTION_BIAS_DRIFT, Config<T>::DIRECTION_ANGLE, measurement_variance,
                Config<T>::POSITION_INTERVAL);

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
void test_impl()
{
        const Track track = generate_track<2, T>();

        ASSERT(!track.position_measurements.empty() && track.position_measurements[0].index == 0);

        const Vector<6, T> position_init_x(
                track.position_measurements[0].position[0], 1.0, -1.0, track.position_measurements[0].position[1], -5,
                0.5);

        const Matrix<6, 6, T> position_init_p = make_diagonal_matrix<6, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10)});

        PositionFilter<T> position_filter(Config<T>::POSITION_FILTER_VARIANCE, position_init_x, position_init_p);

        std::vector<std::optional<Vector<2, T>>> position_filter_result_position;
        position_filter_result_position.reserve(track.points.size());

        NeesAverage<2, T> position_filter_nees_average_position;

        std::optional<std::size_t> last_position_i;
        auto position_iter = track.position_measurements.cbegin();

        for (; position_iter != track.position_measurements.cend(); ++position_iter)
        {
                const PositionMeasurement<2, T>& m = *position_iter;

                ASSERT(!last_position_i || last_position_i < m.index);

                if (last_position_i && m.index > *last_position_i + Config<T>::POSITION_INTERVAL)
                {
                        position_filter_result_position.emplace_back();
                }

                if (!last_position_i)
                {
                        last_position_i = m.index;
                }

                position_filter.predict((m.index - *last_position_i) * Config<T>::DT);
                position_filter.update(m.position, Config<T>::MEASUREMENT_POSITION_VARIANCE);
                last_position_i = m.index;

                position_filter_result_position.push_back(position_filter.position());

                position_filter_nees_average_position.add(
                        track.points[m.index].position, position_filter.position(), position_filter.position_p());

                if (m.index >= 300)
                {
                        break;
                }
        }

        ASSERT(last_position_i && position_iter != track.position_measurements.cend());
        ASSERT(position_iter->index == *last_position_i);

        const typename PositionFilter<T>::Angle angle = position_filter.velocity_angle();

        const T measurement_angle = std::atan2(
                track.process_measurements[*last_position_i].direction[1],
                track.process_measurements[*last_position_i].direction[0]);
        const T angle_difference = normalize_angle_difference(measurement_angle - angle.angle);
        const T angle_r_variance = square(degrees_to_radians(5.0));
        const T angle_variance = angle.variance + Config<T>::MEASUREMENT_DIRECTION_VARIANCE + angle_r_variance;

        LOG("estimated angle = " + to_string(radians_to_degrees(angle.angle))
            + "; measurement angle = " + to_string(radians_to_degrees(measurement_angle)) + "\n"
            + "angle difference = " + to_string(radians_to_degrees(angle_difference))
            + "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(angle_variance))));

        const Vector<9, T> init_x(
                position_iter->position[0], 1.0, -1.0, position_iter->position[1], -5, 0.5, angle_difference, 0, 0);
        const Matrix<9, 9, T> init_p = make_diagonal_matrix<9, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10), angle_variance,
                 square(degrees_to_radians(0.1)), angle_r_variance});

        ProcessFilterEkf<T> process_ekf(
                Config<T>::DT, Config<T>::PROCESS_FILTER_POSITION_VARIANCE, Config<T>::PROCESS_FILTER_ANGLE_VARIANCE,
                Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE, init_x, init_p);

        std::vector<Vector<2, T>> process_ekf_result_position;
        process_ekf_result_position.reserve(track.points.size());

        std::vector<std::optional<T>> process_ekf_result_speed;
        process_ekf_result_speed.reserve(track.points.size());
        process_ekf_result_speed.resize(*last_position_i);

        NeesAverage<2, T> process_ekf_nees_average_position;
        NeesAverage<1, T> process_ekf_nees_average_angle;
        NeesAverage<1, T> process_ekf_nees_average_angle_r;

        ++position_iter;
        ASSERT(position_iter->index > *last_position_i);

        for (std::size_t i = *last_position_i; i < track.points.size(); ++i)
        {
                process_ekf.predict();

                if (position_iter != track.position_measurements.cend() && position_iter->index == i)
                {
                        const auto& measurement = *position_iter;

                        if (i > *last_position_i + Config<T>::POSITION_INTERVAL)
                        {
                                position_filter_result_position.emplace_back();
                        }

                        position_filter.predict((i - *last_position_i) * Config<T>::DT);
                        position_filter.update(measurement.position, Config<T>::MEASUREMENT_POSITION_VARIANCE);

                        position_filter_result_position.push_back(position_filter.position());

                        position_filter_nees_average_position.add(
                                track.points[i].position, position_filter.position(), position_filter.position_p());

                        if (measurement.speed)
                        {
                                process_ekf.update_position_velocity_acceleration(
                                        measurement.position, *measurement.speed,
                                        track.process_measurements[i].direction,
                                        track.process_measurements[i].acceleration,
                                        Config<T>::MEASUREMENT_POSITION_VARIANCE, Config<T>::MEASUREMENT_SPEED_VARIANCE,
                                        Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                                        Config<T>::MEASUREMENT_ACCELERATION_VARIANCE);
                        }
                        else
                        {
                                process_ekf.update_position_direction_acceleration(
                                        measurement.position, track.process_measurements[i].direction,
                                        track.process_measurements[i].acceleration,
                                        Config<T>::MEASUREMENT_POSITION_VARIANCE,
                                        Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                                        Config<T>::MEASUREMENT_ACCELERATION_VARIANCE);
                        }

                        LOG(to_string(i) + ": track = "
                            + to_string(radians_to_degrees(normalize_angle_difference(track.points[i].angle)))
                            + "; process = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_ekf.angle())))
                            + "; speed = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_ekf.angle_speed())))
                            + "; r = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_ekf.angle_r()))));

                        ASSERT(i == position_iter->index);
                        last_position_i = position_iter->index;
                        ++position_iter;
                        ASSERT(position_iter == track.position_measurements.cend()
                               || position_iter->index > *last_position_i);
                }
                else
                {
                        process_ekf.update_acceleration(
                                track.process_measurements[i].acceleration,
                                Config<T>::MEASUREMENT_ACCELERATION_VARIANCE);
                }

                process_ekf_result_position.push_back(process_ekf.position());
                process_ekf_result_speed.push_back(process_ekf.speed());

                process_ekf_nees_average_position.add(
                        track.points[i].position, process_ekf.position(), process_ekf.position_p());
                process_ekf_nees_average_angle.add(track.points[i].angle, process_ekf.angle(), process_ekf.angle_p());
                process_ekf_nees_average_angle_r.add(
                        track.points[i].angle_r, process_ekf.angle_r(), process_ekf.angle_r_p());
        }

        view::write_to_file(
                make_annotation<T>(), track, Config<T>::POSITION_INTERVAL, position_filter_result_position,
                process_ekf_result_speed, process_ekf_result_position);

        LOG("Position Filter Position: " + position_filter_nees_average_position.check_string());
        LOG("Process EKF Position: " + process_ekf_nees_average_position.check_string());
        LOG("Process EKF Angle: " + process_ekf_nees_average_angle.check_string());
        LOG("Process EKF Angle R: " + process_ekf_nees_average_angle_r.check_string());
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

TEST_SMALL("Filter 2D", test)
}
}
