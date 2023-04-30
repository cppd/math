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

#include "show_file.h"
#include "simulator.h"
#include "utility.h"

#include "../filter.h"
#include "../nees.h"

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <map>
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
        static constexpr T POSITION_DT = DT * POSITION_INTERVAL;

        static constexpr T TRACK_VELOCITY_MEAN = 10;
        static constexpr T TRACK_VELOCITY_VARIANCE = square(0.1);

        static constexpr T MEASUREMENT_VELOCITY_AMOUNT_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_VELOCITY_DIRECTION_VARIANCE = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_ACCELERATION_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_POSITION_VARIANCE = square(20.0);
        static constexpr T MEASUREMENT_POSITION_SPEED_VARIANCE = square(0.1);

        static constexpr T PROCESS_VARIANCE = square(0.1);
        static constexpr T PROCESS_POSITION_VARIANCE = POSITION_INTERVAL * PROCESS_VARIANCE;
        static constexpr T DIRECTION_VELOCITY_MEASUREMENT_VARIANCE = square(degrees_to_radians(50.0));
        static constexpr T DIRECTION_PROCESS_DIFFERENCE_VARIANCE = square(degrees_to_radians(0.0001));
};

template <std::size_t N, typename T>
Track<N, T> generate_track()
{
        constexpr std::size_t COUNT = 6000;

        const TrackMeasurementVariance<T> measurement_variance{
                .velocity_amount = Config<T>::MEASUREMENT_VELOCITY_AMOUNT_VARIANCE,
                .velocity_direction = Config<T>::MEASUREMENT_VELOCITY_DIRECTION_VARIANCE,
                .acceleration = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                .position = Config<T>::MEASUREMENT_POSITION_VARIANCE,
                .position_speed = Config<T>::MEASUREMENT_POSITION_SPEED_VARIANCE};

        Track res = generate_track<N, T>(
                COUNT, Config<T>::DT, Config<T>::TRACK_VELOCITY_MEAN, Config<T>::TRACK_VELOCITY_VARIANCE,
                measurement_variance, Config<T>::POSITION_INTERVAL);

        for (auto& [i, p] : res.position_measurements)
        {
                ASSERT(i >= 0 && i < COUNT);
                if ((i >= 1000 && i <= 1600) || (i >= 2600 && i <= 3300) || (i >= 5000 && i <= 5600))
                {
                        p.reset();
                }
        }

        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<Vector<N, T>>> position_measurements(const Track<N, T>& track)
{
        std::vector<std::optional<Vector<N, T>>> res;
        res.reserve(track.position_measurements.size());
        for (const auto& [_, v] : std::map{track.position_measurements.cbegin(), track.position_measurements.cend()})
        {
                if (v)
                {
                        res.push_back(v->position);
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <typename T>
Matrix<2, 2, T> velocity_r(const Matrix<2, 2, T>& measurement_r, const Vector<2, T>& direction, const T amount)
{
        // x = amount*cos(angle)
        // y = amount*sin(angle)
        // Jacobian matrix
        //  cos(angle) -amount*sin(angle)
        //  sin(angle)  amount*cos(angle)
        const T cos = direction[0];
        const T sin = direction[1];
        const Matrix<2, 2, T> error_propagation{
                {cos, -amount * sin},
                {sin,  amount * cos}
        };
        return error_propagation * measurement_r * error_propagation.transposed();
}

template <typename T>
T velocity_angle_p(const Matrix<2, 2, T>& velocity_r, const Vector<2, T>& velocity)
{
        // amount = sqrt(x*x+y*y)
        // angle = atan(y/x)
        // Jacobian matrix
        //  x/sqrt(x*x+y*y) y/sqrt(x*x+y*y)
        //     -y/(x*x+y*y)     x/(x*x+y*y)
        const T norm_squared = velocity.norm_squared();
        const T norm = std::sqrt(norm_squared);
        const T x = velocity[0];
        const T y = velocity[1];
        const Matrix<2, 2, T> error_propagation{
                {         x / norm,         y / norm},
                {-y / norm_squared, x / norm_squared}
        };
        const Matrix<2, 2, T> r = error_propagation * velocity_r * error_propagation.transposed();
        return r(1, 1);
}

template <typename T>
T normalize_angle_difference(T difference)
{
        difference = std::fmod(difference, 2 * PI<T>);
        if (std::abs(difference) <= PI<T>)
        {
                return difference;
        }
        return (difference > 0) ? (difference - 2 * PI<T>) : (difference + 2 * PI<T>);
}

template <typename T>
constexpr Matrix<6, 6, T> create_position_f(const T dt)
{
        const T dt_2 = square(dt) / 2;
        return {
                {1, dt, dt_2, 0,  0,    0},
                {0,  1,   dt, 0,  0,    0},
                {0,  0,    1, 0,  0,    0},
                {0,  0,    0, 1, dt, dt_2},
                {0,  0,    0, 0,  1,   dt},
                {0,  0,    0, 0,  0,    1},
        };
}

template <typename T>
constexpr Matrix<6, 6, T> create_position_q(const T dt, const T process_variance)
{
        const T dt_2 = square(dt) / 2;
        const Matrix<6, 2, T> noise_transition{
                {dt_2,    0},
                {  dt,    0},
                {   1,    0},
                {   0, dt_2},
                {   0,   dt},
                {   0,    1},
        };
        const Matrix<2, 2, T> process_covariance{
                {process_variance,                0},
                {               0, process_variance}
        };
        return noise_transition * process_covariance * noise_transition.transposed();
}

template <typename T>
constexpr Matrix<2, 2, T> create_difference_f()
{
        return {
                {1, Config<T>::DT},
                {0,             1}
        };
}

template <typename T>
constexpr Matrix<2, 2, T> create_difference_q()
{
        const Matrix<2, 1, T> noise_transition{
                {square(Config<T>::DT) / 2},
                {Config<T>::DT},
        };
        const Matrix<1, 1, T> process_covariance{{Config<T>::DIRECTION_PROCESS_DIFFERENCE_VARIANCE}};
        return noise_transition * process_covariance * noise_transition.transposed();
}

template <typename T>
void test_impl()
{
        constexpr std::size_t N = 6;
        constexpr std::size_t M = 2;

        constexpr Vector<N, T> X(10, 0, 0, 10, -5, 0.5);
        constexpr Matrix<N, N, T> P{
                {square(25),          0,         0,          0,          0,         0},
                {         0, square(10),         0,          0,          0,         0},
                {         0,          0, square(3),          0,          0,         0},
                {         0,          0,         0, square(25),          0,         0},
                {         0,          0,         0,          0, square(10),         0},
                {         0,          0,         0,          0,          0, square(3)}
        };

        constexpr Vector<2, T> DIFFERENCE_X(1, 1);
        constexpr Matrix<2, 2, T> DIFFERENCE_P{
                {square(3),         0},
                {        0, square(1)}
        };

        constexpr Matrix<N, N, T> F = create_position_f<T>(Config<T>::DT);
        constexpr Matrix<N, N, T> F_T = F.transposed();
        constexpr Matrix<N, N, T> Q = create_position_q(Config<T>::DT, Config<T>::PROCESS_VARIANCE);

        constexpr Matrix<N, N, T> POSITION_F = create_position_f<T>(Config<T>::POSITION_DT);
        constexpr Matrix<N, N, T> POSITION_F_T = POSITION_F.transposed();
        constexpr Matrix<N, N, T> POSITION_Q =
                create_position_q(Config<T>::POSITION_DT, Config<T>::PROCESS_POSITION_VARIANCE);

        constexpr Matrix<M, N, T> VELOCITY_H{
                {0, 1, 0, 0, 0, 0},
                {0, 0, 0, 0, 1, 0}
        };
        constexpr Matrix<N, M, T> VELOCITY_H_T = VELOCITY_H.transposed();
        constexpr Matrix<M, M, T> VELOCITY_MEASUREMENT_R{
                {Config<T>::MEASUREMENT_VELOCITY_AMOUNT_VARIANCE,                                                  0},
                {                                              0, Config<T>::DIRECTION_VELOCITY_MEASUREMENT_VARIANCE}
        };

        constexpr Matrix<M, N, T> POSITION_H{
                {1, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0}
        };
        constexpr Matrix<N, M, T> POSITION_H_T = POSITION_H.transposed();
        constexpr Matrix<M, M, T> POSITION_R = {
                {Config<T>::MEASUREMENT_POSITION_VARIANCE,                                        0},
                {                                       0, Config<T>::MEASUREMENT_POSITION_VARIANCE}
        };

        constexpr Matrix<2, 2, T> DIFFERENCE_F = create_difference_f<T>();
        constexpr Matrix<2, 2, T> DIFFERENCE_F_T = DIFFERENCE_F.transposed();
        constexpr Matrix<2, 2, T> DIFFERENCE_Q = create_difference_q<T>();
        constexpr Matrix<1, 2, T> DIFFERENCE_H{
                {1, 0}
        };
        constexpr Matrix<2, 1, T> DIFFERENCE_H_T = DIFFERENCE_H.transposed();

        const Track track = generate_track<2, T>();

        Filter<N, T> process_filter(X, P);
        Filter<N, T> position_filter(process_filter.x(), process_filter.p());
        Filter<2, T> difference_filter(DIFFERENCE_X, DIFFERENCE_P);

        std::vector<std::optional<Vector<2, T>>> position_result;
        position_result.reserve(track.positions.size());
        std::vector<Vector<2, T>> result;
        result.reserve(track.positions.size());

        NeesAverage<2, T> nees_average;

        std::optional<T> filtered_difference;
        std::optional<T> position_velocity_angle;
        std::optional<T> position_velocity_angle_p;

        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                if (const auto iter = track.position_measurements.find(i); iter != track.position_measurements.cend())
                {
                        position_filter.predict(POSITION_F, POSITION_F_T, POSITION_Q);

                        if (iter->second)
                        {
                                position_filter.update(POSITION_H, POSITION_H_T, POSITION_R, iter->second->position);

                                if (i >= 200)
                                {
                                        const Vector<M, T> velocity(position_filter.x()[1], position_filter.x()[4]);
                                        const T velocity_angle = std::atan2(velocity[1], velocity[0]);
                                        position_velocity_angle = velocity_angle;
                                        const Matrix<M, M, T> velocity_p{
                                                {position_filter.p()(1, 1), position_filter.p()(1, 4)},
                                                {position_filter.p()(4, 1), position_filter.p()(4, 4)}
                                        };
                                        position_velocity_angle_p = velocity_angle_p(velocity_p, velocity);
                                }
                                position_result.push_back(Vector<2, T>(position_filter.x()[0], position_filter.x()[3]));
                        }
                        else
                        {
                                position_velocity_angle.reset();
                                position_velocity_angle_p.reset();
                                position_result.push_back(std::nullopt);
                        }
                }

                if (const auto& p = position_velocity_angle)
                {
                        const T measurement_angle = std::atan2(
                                track.process_measurements[i].direction[1], track.process_measurements[i].direction[0]);
                        const T difference = normalize_angle_difference(*p - measurement_angle);
                        difference_filter.predict(DIFFERENCE_F, DIFFERENCE_F_T, DIFFERENCE_Q);
                        difference_filter.update(
                                DIFFERENCE_H, DIFFERENCE_H_T,
                                {{*position_velocity_angle_p + Config<T>::MEASUREMENT_VELOCITY_DIRECTION_VARIANCE}},
                                Vector<1, T>(difference));
                        filtered_difference = difference_filter.x()[0];
                }

                process_filter.predict(F, F_T, Q);

                if (const auto iter = track.position_measurements.find(i);
                    iter != track.position_measurements.cend() && iter->second)
                {
                        process_filter.update(POSITION_H, POSITION_H_T, POSITION_R, iter->second->position);
                }
                else if (const auto& difference = filtered_difference)
                {
                        const auto& direction = rotate(track.process_measurements[i].direction, *difference);
                        const auto& amount = track.process_measurements[i].amount;
                        const Vector<M, T> velocity = direction * amount;
                        const Matrix<M, M, T> r = velocity_r(VELOCITY_MEASUREMENT_R, direction, amount);
                        process_filter.update(VELOCITY_H, VELOCITY_H_T, r, velocity);
                }

                result.push_back({process_filter.x()[0], process_filter.x()[3]});

                nees_average.add(
                        track.positions[i], Vector<2, T>(process_filter.x()[0], process_filter.x()[3]),
                        {
                                {process_filter.p()(0, 0), process_filter.p()(0, 3)},
                                {process_filter.p()(3, 0), process_filter.p()(3, 3)}
                });
        }

        std::vector<Vector<2, T>> measurement_angles;
        measurement_angles.reserve(track.positions.size());
        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                const T angle = std::atan2(
                        track.process_measurements[i].direction[1], track.process_measurements[i].direction[0]);
                measurement_angles.emplace_back(track.positions[i][0], -500 + radians_to_degrees(angle));
        }

        write_to_file(track.positions, measurement_angles, position_measurements(track), position_result, result);

        LOG(nees_average.check_string());
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
