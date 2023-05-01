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

template <std::size_t N, typename T>
std::vector<Vector<N, T>> angle_measurements(const Track<N, T>& track, const T offset)
{
        std::vector<Vector<2, T>> res;
        res.reserve(track.positions.size());
        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                const T vx = track.process_measurements[i].direction[0];
                const T vy = track.process_measurements[i].direction[1];
                const T angle = -std::atan2(vy, vx);
                res.emplace_back(track.positions[i][0], offset + radians_to_degrees(angle));
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
        // angle = atan(y/x)
        // Jacobian matrix
        //  -y/(x*x+y*y) x/(x*x+y*y)
        const T norm_squared = velocity.norm_squared();
        const T x = velocity[0];
        const T y = velocity[1];
        const Matrix<1, 2, T> error_propagation{
                {-y / norm_squared, x / norm_squared}
        };
        const Matrix<1, 1, T> r = error_propagation * velocity_r * error_propagation.transposed();
        return r(0, 0);
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
struct Angle final
{
        T angle;
        T variance;
};

template <typename T>
class PositionFilter final
{
        static constexpr std::size_t N = 6;
        static constexpr std::size_t M = 2;

        static constexpr Matrix<N, N, T> F = create_position_f<T>(Config<T>::POSITION_DT);
        static constexpr Matrix<N, N, T> F_T = F.transposed();
        static constexpr Matrix<N, N, T> Q =
                create_position_q<T>(Config<T>::POSITION_DT, Config<T>::PROCESS_POSITION_VARIANCE);

        static constexpr Matrix<M, N, T> POSITION_H{
                {1, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0}
        };
        static constexpr Matrix<N, M, T> POSITION_H_T = POSITION_H.transposed();
        static constexpr Matrix<M, M, T> POSITION_R = make_diagonal_matrix<M, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, Config<T>::MEASUREMENT_POSITION_VARIANCE});

        Filter<N, T> filter_;

public:
        PositionFilter(const Vector<N, T> init_x, const Matrix<N, N, T>& init_p)
                : filter_(init_x, init_p)
        {
        }

        void predict()
        {
                filter_.predict(F, F_T, Q);
        }

        void update(const Vector<M, T>& position)
        {
                filter_.update(POSITION_H, POSITION_H_T, POSITION_R, position);
        }

        [[nodiscard]] Vector<M, T> position() const
        {
                return {filter_.x()[0], filter_.x()[3]};
        }

        [[nodiscard]] Matrix<M, M, T> position_p() const
        {
                return {
                        {filter_.p()(0, 0), filter_.p()(0, 3)},
                        {filter_.p()(3, 0), filter_.p()(3, 3)}
                };
        }

        [[nodiscard]] Angle<T> velocity_angle() const
        {
                const Vector<M, T> velocity{filter_.x()[1], filter_.x()[4]};
                const Matrix<M, M, T> velocity_p{
                        {filter_.p()(1, 1), filter_.p()(1, 4)},
                        {filter_.p()(4, 1), filter_.p()(4, 4)}
                };
                return {.angle = std::atan2(velocity[1], velocity[0]),
                        .variance = velocity_angle_p(velocity_p, velocity)};
        }
};

template <typename T>
class ProcessFilter final
{
        static constexpr std::size_t N = 6;
        static constexpr std::size_t M = 2;

        static constexpr Matrix<N, N, T> F = create_position_f<T>(Config<T>::DT);
        static constexpr Matrix<N, N, T> F_T = F.transposed();
        static constexpr Matrix<N, N, T> Q = create_position_q<T>(Config<T>::DT, Config<T>::PROCESS_VARIANCE);

        static constexpr Matrix<M, N, T> POSITION_H{
                {1, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0}
        };
        static constexpr Matrix<N, M, T> POSITION_H_T = POSITION_H.transposed();
        static constexpr Matrix<M, M, T> POSITION_R = make_diagonal_matrix<M, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, Config<T>::MEASUREMENT_POSITION_VARIANCE});

        static constexpr Matrix<M, N, T> VELOCITY_H{
                {0, 1, 0, 0, 0, 0},
                {0, 0, 0, 0, 1, 0}
        };
        static constexpr Matrix<N, M, T> VELOCITY_H_T = VELOCITY_H.transposed();
        static constexpr Matrix<M, M, T> VELOCITY_MEASUREMENT_R = make_diagonal_matrix<M, T>(
                {Config<T>::MEASUREMENT_VELOCITY_AMOUNT_VARIANCE, Config<T>::DIRECTION_VELOCITY_MEASUREMENT_VARIANCE});

        Filter<N, T> filter_;

public:
        ProcessFilter(const Vector<N, T> init_x, const Matrix<N, N, T>& init_p)
                : filter_(init_x, init_p)
        {
        }

        void predict()
        {
                filter_.predict(F, F_T, Q);
        }

        void update_position(const Vector<M, T>& position)
        {
                filter_.update(POSITION_H, POSITION_H_T, POSITION_R, position);
        }

        void update_velocity(const Vector<M, T>& direction, const T amount)
        {
                const Vector<M, T> velocity = direction * amount;
                const Matrix<M, M, T> r = velocity_r(VELOCITY_MEASUREMENT_R, direction, amount);
                filter_.update(VELOCITY_H, VELOCITY_H_T, r, velocity);
        }

        [[nodiscard]] Vector<M, T> position() const
        {
                return {filter_.x()[0], filter_.x()[3]};
        }

        [[nodiscard]] Matrix<M, M, T> position_p() const
        {
                return {
                        {filter_.p()(0, 0), filter_.p()(0, 3)},
                        {filter_.p()(3, 0), filter_.p()(3, 3)}
                };
        }
};

template <typename T>
class DifferenceFilter final
{
        static constexpr std::size_t N = 2;
        static constexpr std::size_t M = 1;

        static constexpr Matrix<N, N, T> create_f()
        {
                return {
                        {1, Config<T>::DT},
                        {0,             1}
                };
        }

        static constexpr Matrix<N, N, T> create_q()
        {
                const Matrix<2, 1, T> noise_transition{
                        {square(Config<T>::DT) / 2},
                        {Config<T>::DT},
                };
                const Matrix<1, 1, T> process_covariance{{Config<T>::DIRECTION_PROCESS_DIFFERENCE_VARIANCE}};
                return noise_transition * process_covariance * noise_transition.transposed();
        }

        static constexpr Vector<N, T> INIT_X{1, 1};
        static constexpr Matrix<N, N, T> INIT_P{
                {square(3),         0},
                {        0, square(3)}
        };

        static constexpr Matrix<N, N, T> F = create_f();
        static constexpr Matrix<N, N, T> F_T = F.transposed();
        static constexpr Matrix<N, N, T> Q = create_q();

        static constexpr Matrix<M, N, T> H{
                {1, 0}
        };
        static constexpr Matrix<N, M, T> H_T = H.transposed();

        Filter<N, T> filter_{INIT_X, INIT_P};

public:
        void predict_and_update(const T difference, const T variance)
        {
                filter_.predict(F, F_T, Q);
                filter_.update(H, H_T, {{variance}}, Vector<1, T>(difference));
        }

        [[nodiscard]] T difference() const
        {
                return filter_.x()[0];
        }

        [[nodiscard]] T difference_p() const
        {
                return filter_.p()(0, 0);
        }
};

template <typename T>
void test_impl()
{
        constexpr std::size_t N = 6;

        const Track track = generate_track<2, T>();

        const Vector<N, T> init_x(track.positions[0][0] + 50, 1.0, -1.0, track.positions[0][1] - 50, -5, 0.5);
        const Matrix<N, N, T> init_p =
                make_diagonal_matrix<N, T>({square(100), square(30), square(10), square(100), square(30), square(10)});

        PositionFilter<T> position_filter(init_x, init_p);
        ProcessFilter<T> process_filter(init_x, init_p);
        DifferenceFilter<T> difference_filter;

        std::vector<std::optional<Vector<2, T>>> position_result;
        position_result.reserve(track.positions.size());
        std::vector<Vector<2, T>> process_result;
        process_result.reserve(track.positions.size());

        NeesAverage<2, T> position_nees_average;
        NeesAverage<2, T> process_nees_average;
        NeesAverage<1, T> difference_nees_average;

        std::optional<T> filtered_difference;

        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                if (const auto iter = track.position_measurements.find(i); iter != track.position_measurements.cend())
                {
                        position_filter.predict();

                        if (const auto& measurement = iter->second)
                        {
                                position_filter.update(measurement->position);

                                position_result.push_back(position_filter.position());
                                position_nees_average.add(
                                        track.positions[i], position_filter.position(), position_filter.position_p());

                                if (i >= 200)
                                {
                                        const Angle<T> position_angle = position_filter.velocity_angle();
                                        const T measurement_angle = std::atan2(
                                                track.process_measurements[i].direction[1],
                                                track.process_measurements[i].direction[0]);
                                        const T difference =
                                                normalize_angle_difference(measurement_angle - position_angle.angle);
                                        const T variance = Config<T>::MEASUREMENT_VELOCITY_DIRECTION_VARIANCE
                                                           + position_angle.variance;

                                        difference_filter.predict_and_update(difference, variance);

                                        filtered_difference = difference_filter.difference();

                                        difference_nees_average.add(
                                                normalize_angle_difference(track.angles[i]),
                                                difference_filter.difference(), difference_filter.difference_p());
                                }
                        }
                        else
                        {
                                position_result.push_back(std::nullopt);
                        }
                }

                process_filter.predict();

                if (const auto iter = track.position_measurements.find(i);
                    iter != track.position_measurements.cend() && iter->second)
                {
                        process_filter.update_position(iter->second->position);
                }
                else if (const auto& difference = filtered_difference)
                {
                        const auto& direction = rotate(track.process_measurements[i].direction, -*difference);
                        const auto& amount = track.process_measurements[i].amount;
                        process_filter.update_velocity(direction, amount);
                }

                process_result.push_back(process_filter.position());
                process_nees_average.add(track.positions[i], process_filter.position(), process_filter.position_p());
        }

        write_to_file(
                track.positions, angle_measurements(track, /*offset=*/T{-500}), position_measurements(track),
                position_result, process_result);

        LOG("Position Filter: " + position_nees_average.check_string());
        LOG("Process Filter: " + process_nees_average.check_string());
        LOG("Difference Filter: " + difference_nees_average.check_string());
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
