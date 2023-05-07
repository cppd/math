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
        static constexpr T POSITION_DT = POSITION_INTERVAL * DT;

        static constexpr T TRACK_VELOCITY_MEAN = 10;
        static constexpr T TRACK_VELOCITY_VARIANCE = square(0.1);

        static constexpr T MEASUREMENT_DIRECTION_VARIANCE = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_ACCELERATION_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_POSITION_VARIANCE = square(20.0);
        static constexpr T MEASUREMENT_POSITION_SPEED_VARIANCE = square(1.0);

        static constexpr T ESTIMATION_FILTER_VARIANCE = square(0.2);

        static constexpr T POSITION_VARIANCE = square(0.2);
        static constexpr T DIFFERENCE_VARIANCE = square(degrees_to_radians(0.001));
};

template <std::size_t N, typename T>
Track<N, T> generate_track()
{
        constexpr std::size_t COUNT = 6000;

        const TrackMeasurementVariance<T> measurement_variance{
                .direction = Config<T>::MEASUREMENT_DIRECTION_VARIANCE,
                .acceleration = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE,
                .position = Config<T>::MEASUREMENT_POSITION_VARIANCE,
                .position_speed = Config<T>::MEASUREMENT_POSITION_SPEED_VARIANCE};

        Track res = generate_track<N, T>(
                COUNT, Config<T>::DT, Config<T>::TRACK_VELOCITY_MEAN, Config<T>::TRACK_VELOCITY_VARIANCE,
                measurement_variance, Config<T>::POSITION_INTERVAL);

        for (auto& [i, p] : res.position_measurements)
        {
                ASSERT(i >= 0 && i < COUNT);
                const auto n = std::llround(i / T{300});
                if ((n > 3) && ((n % 5) == 0))
                {
                        p.reset();
                }
                if (p && (std::llround(i / T{100}) % 3 != 0))
                {
                        p->speed.reset();
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
std::vector<std::optional<Vector<N, T>>> speed_measurements(const Track<N, T>& track, const T offset)
{
        std::vector<std::optional<Vector<N, T>>> res;
        res.reserve(track.position_measurements.size());
        for (const auto& [i, v] : std::map{track.position_measurements.cbegin(), track.position_measurements.cend()})
        {
                if (v && v->speed)
                {
                        res.emplace_back(Vector<2, T>(track.positions[i][0], offset + (*v->speed)));
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

template <std::size_t N, typename T>
std::vector<Vector<N, T>> acceleration_measurements(const Track<N, T>& track, const std::size_t index, const T offset)
{
        ASSERT(index < N);
        std::vector<Vector<2, T>> res;
        res.reserve(track.positions.size());
        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                res.emplace_back(track.positions[i][0], offset + track.process_measurements[i].acceleration[index]);
        }
        return res;
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
struct Angle final
{
        T angle;
        T variance;
};

template <typename T>
class EstimationFilter final
{
        static constexpr std::size_t N = 6;
        static constexpr std::size_t M = 2;

        static constexpr Matrix<N, N, T> F = []()
        {
                const T dt = Config<T>::POSITION_DT;
                const T dt_2 = square(dt) / 2;
                return Matrix<N, N, T>{
                        {1, dt, dt_2, 0,  0,    0},
                        {0,  1,   dt, 0,  0,    0},
                        {0,  0,    1, 0,  0,    0},
                        {0,  0,    0, 1, dt, dt_2},
                        {0,  0,    0, 0,  1,   dt},
                        {0,  0,    0, 0,  0,    1}
                };
        }();

        static constexpr Matrix<N, N, T> F_T = F.transposed();

        static constexpr Matrix<N, N, T> Q = []()
        {
                const T dt = Config<T>::POSITION_DT;
                const T dt_2 = square(dt) / 2;
                const Matrix<N, 2, T> noise_transition{
                        {dt_2,    0},
                        {  dt,    0},
                        {   1,    0},
                        {   0, dt_2},
                        {   0,   dt},
                        {   0,    1},
                };

                const T p = Config<T>::ESTIMATION_FILTER_VARIANCE;
                const Matrix<2, 2, T> process_covariance{
                        {p, 0},
                        {0, p}
                };

                return noise_transition * process_covariance * noise_transition.transposed();
        }();

        static constexpr Matrix<M, N, T> H{
                {1, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0}
        };
        static constexpr Matrix<N, M, T> H_T = H.transposed();
        static constexpr Matrix<2, 2, T> R = []
        {
                const T pv = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                return Matrix<2, 2, T>{
                        {pv,  0},
                        { 0, pv}
                };
        }();

        static T velocity_angle_p(const Matrix<2, 2, T>& velocity_r, const Vector<2, T>& velocity)
        {
                // angle = atan(y/x)
                // Jacobian
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

        Filter<N, T> filter_;

public:
        EstimationFilter(const Vector<N, T> init_x, const Matrix<N, N, T>& init_p)
                : filter_(init_x, init_p)
        {
        }

        void predict()
        {
                filter_.predict(F, F_T, Q);
        }

        void update(const Vector<M, T>& position)
        {
                filter_.update(H, H_T, R, position);
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
        static constexpr std::size_t N = 8;

        static constexpr Matrix<N, N, T> F = []()
        {
                const T dt = Config<T>::DT;
                const T dt_2 = square(dt) / 2;
                return Matrix<N, N, T>{
                        {1, dt, dt_2, 0,  0,    0, 0,  0},
                        {0,  1,   dt, 0,  0,    0, 0,  0},
                        {0,  0,    1, 0,  0,    0, 0,  0},
                        {0,  0,    0, 1, dt, dt_2, 0,  0},
                        {0,  0,    0, 0,  1,   dt, 0,  0},
                        {0,  0,    0, 0,  0,    1, 0,  0},
                        {0,  0,    0, 0,  0,    0, 1, dt},
                        {0,  0,    0, 0,  0,    0, 0,  1}
                };
        }();

        static constexpr Matrix<N, N, T> F_T = F.transposed();

        static constexpr Matrix<N, N, T> Q = []()
        {
                const T dt = Config<T>::DT;
                const T dt_2 = square(dt) / 2;
                const Matrix<N, 3, T> noise_transition{
                        {dt_2,    0,    0},
                        {  dt,    0,    0},
                        {   1,    0,    0},
                        {   0, dt_2,    0},
                        {   0,   dt,    0},
                        {   0,    1,    0},
                        {   0,    0, dt_2},
                        {   0,    0,   dt}
                };

                const T p = Config<T>::POSITION_VARIANCE;
                const T d = Config<T>::DIFFERENCE_VARIANCE;
                const Matrix<3, 3, T> process_covariance{
                        {p, 0, 0},
                        {0, p, 0},
                        {0, 0, d}
                };

                return noise_transition * process_covariance * noise_transition.transposed();
        }();

        static constexpr Matrix<2, N, T> POSITION_H{
                {1, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0, 0, 0}
        };
        static constexpr Matrix<N, 2, T> POSITION_H_T = POSITION_H.transposed();
        static constexpr Matrix<2, 2, T> POSITION_R = []
        {
                const T pv = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                return Matrix<2, 2, T>{
                        {pv,  0},
                        { 0, pv}
                };
        }();

        static Matrix<6, 6, T> position_velocity_acceleration_r(const Vector<2, T>& direction, const T speed)
        {
                const T pv = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                const T sv = Config<T>::MEASUREMENT_POSITION_SPEED_VARIANCE;
                const T dv = Config<T>::MEASUREMENT_DIRECTION_VARIANCE;
                const T av = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE;
                const Matrix<6, 6, T> r{
                        {pv,  0,  0,  0,  0,  0},
                        { 0, pv,  0,  0,  0,  0},
                        { 0,  0, sv,  0,  0,  0},
                        { 0,  0,  0, dv,  0,  0},
                        { 0,  0,  0,  0, av,  0},
                        { 0,  0,  0,  0,  0, av}
                };

                // px = px
                // py = py
                // vx = speed*cos(angle)
                // vy = speed*sin(angle)
                // ax = ax
                // ay = ay
                // Jacobian
                const T cos = direction[0];
                const T sin = direction[1];
                const Matrix<6, 6, T> error_propagation{
                        {1, 0,   0,            0, 0, 0},
                        {0, 1,   0,            0, 0, 0},
                        {0, 0, cos, -speed * sin, 0, 0},
                        {0, 0, sin,  speed * cos, 0, 0},
                        {0, 0,   0,            0, 1, 0},
                        {0, 0,   0,            0, 0, 1}
                };

                return error_propagation * r * error_propagation.transposed();
        }

        static Vector<6, T> position_velocity_acceleration_h(const Vector<N, T>& x)
        {
                // x = px
                // y = py
                // dx = vx*cos(angle) - vy*sin(angle)
                // dy = vx*sin(angle) + vy*cos(angle)
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                const T px = x[0];
                const T vx = x[1];
                const T ax = x[2];
                const T py = x[3];
                const T vy = x[4];
                const T ay = x[5];
                const T angle = x[6];
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {px, py, vx * cos - vy * sin, vx * sin + vy * cos, ax * cos - ay * sin, ax * sin + ay * cos};
        }

        static Matrix<6, N, T> position_velocity_acceleration_hj(const Vector<N, T>& x)
        {
                // x = px
                // y = py
                // dx = vx*cos(angle) - vy*sin(angle)
                // dy = vx*sin(angle) + vy*cos(angle)
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                // Jacobian
                const T vx = x[1];
                const T vy = x[4];
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {
                        {1,   0,   0, 0,    0,    0,                    0, 0},
                        {0,   0,   0, 1,    0,    0,                    0, 0},
                        {0, cos,   0, 0, -sin,    0, -vx * sin - vy * cos, 0},
                        {0, sin,   0, 0,  cos,    0,  vx * cos - vy * sin, 0},
                        {0,   0, cos, 0,    0, -sin, -ax * sin - ay * cos, 0},
                        {0,   0, sin, 0,    0,  cos,  ax * cos - ay * sin, 0}
                };
        }

        static Matrix<2, 2, T> acceleration_r()
        {
                const T av = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE;
                return {
                        {av,  0},
                        { 0, av}
                };
        }

        static Vector<2, T> acceleration_h(const Vector<N, T>& x)
        {
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {ax * cos - ay * sin, ax * sin + ay * cos};
        }

        static Matrix<2, N, T> acceleration_hj(const Vector<N, T>& x)
        {
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                // Jacobian
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {
                        {0, 0, cos, 0, 0, -sin, -ax * sin - ay * cos, 0},
                        {0, 0, sin, 0, 0,  cos,  ax * cos - ay * sin, 0}
                };
        }

        static Matrix<6, 6, T> position_direction_acceleration_r(const Vector<2, T>& direction)
        {
                const T pv = Config<T>::MEASUREMENT_POSITION_VARIANCE;
                const T dv = Config<T>::MEASUREMENT_DIRECTION_VARIANCE;
                const T av = Config<T>::MEASUREMENT_ACCELERATION_VARIANCE;
                const Matrix<5, 5, T> r{
                        {pv,  0,  0,  0,  0},
                        { 0, pv,  0,  0,  0},
                        { 0,  0, dv,  0,  0},
                        { 0,  0,  0, av,  0},
                        { 0,  0,  0,  0, av}
                };

                // dx = cos(angle)
                // dy = sin(angle)
                // ax = ax
                // ay = ay
                // Jacobian
                const T cos = direction[0];
                const T sin = direction[1];
                const Matrix<6, 5, T> error_propagation{
                        {1, 0,    0, 0, 0},
                        {0, 1,    0, 0, 0},
                        {0, 0, -sin, 0, 0},
                        {0, 0,  cos, 0, 0},
                        {0, 0,    0, 1, 0},
                        {0, 0,    0, 0, 1}
                };

                return error_propagation * r * error_propagation.transposed();
        }

        static Vector<6, T> position_direction_acceleration_h(const Vector<N, T>& x)
        {
                // px = px
                // py = py
                // dx = (vx*cos(angle) - vy*sin(angle)) / sqrt(vx*vx + vy*vy);
                // dy = (vx*sin(angle) + vy*cos(angle)) / sqrt(vx*vx + vy*vy);
                // ax = (ax*cos(angle) - ay*sin(angle))
                // ay = (ax*sin(angle) + ay*cos(angle))
                const T px = x[0];
                const T vx = x[1];
                const T ax = x[2];
                const T py = x[3];
                const T vy = x[4];
                const T ay = x[5];
                const T angle = x[6];
                const T speed = std::sqrt(square(vx) + square(vy));
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {px,
                        py,
                        (vx * cos - vy * sin) / speed,
                        (vx * sin + vy * cos) / speed,
                        ax * cos - ay * sin,
                        ax * sin + ay * cos};
        }

        static Matrix<6, N, T> position_direction_acceleration_hj(const Vector<N, T>& x)
        {
                // px = px
                // py = py
                // dx = (vx*cos(angle) - vy*sin(angle)) / sqrt(vx*vx + vy*vy);
                // dy = (vx*sin(angle) + vy*cos(angle)) / sqrt(vx*vx + vy*vy);
                // ax = (ax*cos(angle) - ay*sin(angle))
                // ay = (ax*sin(angle) + ay*cos(angle))
                // Jacobian
                // mPx=Px;
                // mPy=Py;
                // mDx=(Vx*Cos[Angle]-Vy*Sin[Angle])/Sqrt[Vx*Vx+Vy*Vy];
                // mDy=(Vx*Sin[Angle]+Vy*Cos[Angle])/Sqrt[Vx*Vx+Vy*Vy];
                // mAx=(Ax*Cos[Angle]-Ay*Sin[Angle]);
                // mAy=(Ax*Sin[Angle]+Ay*Cos[Angle]);
                // Simplify[D[{mPx,mPy,mDx,mDy,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Angle,AngleV}}]]
                const T vx = x[1];
                const T vy = x[4];
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T l = std::sqrt(square(vx) + square(vy));
                const T l_3 = power<3>(l);
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                const T d_1 = vy * cos + vx * sin;
                const T d_2 = vx * cos - vy * sin;
                const T a_1 = -ax * sin - ay * cos;
                const T a_2 = ax * cos - ay * sin;
                return {
                        {1,               0,   0, 0,               0,    0,        0, 0},
                        {0,               0,   0, 1,               0,    0,        0, 0},
                        {0,  vy * d_1 / l_3,   0, 0, -vx * d_1 / l_3,    0, -d_1 / l, 0},
                        {0, -vy * d_2 / l_3,   0, 0,  vx * d_2 / l_3,    0,  d_2 / l, 0},
                        {0,               0, cos, 0,               0, -sin,      a_1, 0},
                        {0,               0, sin, 0,               0,  cos,      a_2, 0}
                };
        }

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

        void update_position(const Vector<2, T>& position)
        {
                filter_.update(POSITION_H, POSITION_H_T, POSITION_R, position);
        }

        void update_position_velocity_acceleration(
                const Vector<2, T>& position,
                const Vector<2, T>& direction,
                const T speed,
                const Vector<2, T>& acceleration)
        {
                filter_.update(
                        position_velocity_acceleration_h, position_velocity_acceleration_hj,
                        position_velocity_acceleration_r(direction, speed),
                        Vector<6, T>(
                                position[0], position[1], direction[0] * speed, direction[1] * speed, acceleration[0],
                                acceleration[1]));
        }

        void update_position_direction_acceleration(
                const Vector<2, T>& position,
                const Vector<2, T>& direction,
                const Vector<2, T>& acceleration)
        {
                filter_.update(
                        position_direction_acceleration_h, position_direction_acceleration_hj,
                        position_direction_acceleration_r(direction),
                        Vector<6, T>(
                                position[0], position[1], direction[0], direction[1], acceleration[0],
                                acceleration[1]));
        }

        void update_acceleration(const Vector<2, T>& acceleration)
        {
                filter_.update(acceleration_h, acceleration_hj, acceleration_r(), acceleration);
        }

        [[nodiscard]] Vector<2, T> position() const
        {
                return {filter_.x()[0], filter_.x()[3]};
        }

        [[nodiscard]] Matrix<2, 2, T> position_p() const
        {
                return {
                        {filter_.p()(0, 0), filter_.p()(0, 3)},
                        {filter_.p()(3, 0), filter_.p()(3, 3)}
                };
        }

        [[nodiscard]] T angle() const
        {
                return filter_.x()[6];
        }

        [[nodiscard]] T angle_speed() const
        {
                return filter_.x()[7];
        }

        [[nodiscard]] T angle_p() const
        {
                return filter_.p()(6, 6);
        }
};

template <typename T>
void test_impl()
{
        const Track track = generate_track<2, T>();

        ASSERT(track.position_measurements.contains(0) && track.position_measurements.find(0)->second);

        const Vector<6, T> position_init_x(
                track.position_measurements.find(0)->second->position[0], 1.0, -1.0,
                track.position_measurements.find(0)->second->position[1], -5, 0.5);

        const Matrix<6, 6, T> position_init_p = make_diagonal_matrix<6, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10)});

        EstimationFilter<T> estimation_filter(position_init_x, position_init_p);

        std::vector<std::optional<Vector<2, T>>> estimation_result;
        estimation_result.reserve(track.positions.size());

        NeesAverage<2, T> estimation_nees_average;

        std::size_t i = 0;

        for (; i < track.positions.size(); ++i)
        {
                if (const auto iter = track.position_measurements.find(i); iter != track.position_measurements.cend())
                {
                        estimation_filter.predict();

                        if (const auto& measurement = iter->second)
                        {
                                estimation_filter.update(measurement->position);

                                estimation_result.push_back(estimation_filter.position());
                                estimation_nees_average.add(
                                        track.positions[i], estimation_filter.position(),
                                        estimation_filter.position_p());

                                if (i >= 300)
                                {
                                        break;
                                }
                        }
                        else
                        {
                                estimation_result.push_back(std::nullopt);
                        }
                }
        }

        const Angle<T> angle = estimation_filter.velocity_angle();

        const T measurement_angle =
                std::atan2(track.process_measurements[i].direction[1], track.process_measurements[i].direction[0]);
        const T angle_difference = normalize_angle_difference(measurement_angle - angle.angle);
        const T angle_variance = angle.variance + Config<T>::MEASUREMENT_DIRECTION_VARIANCE;

        LOG("estimated angle = " + to_string(radians_to_degrees(angle.angle))
            + "; measurement angle = " + to_string(radians_to_degrees(measurement_angle)) + "\n"
            + "angle difference = " + to_string(radians_to_degrees(angle_difference))
            + "; angle stddev = " + to_string(radians_to_degrees(std::sqrt(angle_variance))));

        const Vector<8, T> init_x(
                track.position_measurements.find(i)->second->position[0], 1.0, -1.0,
                track.position_measurements.find(i)->second->position[1], -5, 0.5, angle_difference, 0);
        const Matrix<8, 8, T> init_p = make_diagonal_matrix<8, T>(
                {Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10),
                 Config<T>::MEASUREMENT_POSITION_VARIANCE, square(30), square(10), angle_variance,
                 square(degrees_to_radians(0.1))});

        ProcessFilter<T> process_filter(init_x, init_p);

        std::vector<Vector<2, T>> process_result;
        process_result.reserve(track.positions.size());

        NeesAverage<2, T> position_nees_average;
        NeesAverage<1, T> angle_nees_average;

        for (; i < track.positions.size(); ++i)
        {
                process_filter.predict();

                if (const auto iter = track.position_measurements.find(i); iter != track.position_measurements.cend())
                {
                        estimation_filter.predict();

                        if (const auto& measurement = iter->second)
                        {
                                estimation_filter.update(measurement->position);

                                estimation_result.push_back(estimation_filter.position());
                                estimation_nees_average.add(
                                        track.positions[i], estimation_filter.position(),
                                        estimation_filter.position_p());

                                if (measurement->speed)
                                {
                                        process_filter.update_position_velocity_acceleration(
                                                measurement->position, track.process_measurements[i].direction,
                                                *measurement->speed, track.process_measurements[i].acceleration);
                                }
                                else
                                {
                                        process_filter.update_position_direction_acceleration(
                                                measurement->position, track.process_measurements[i].direction,
                                                track.process_measurements[i].acceleration);
                                }

                                LOG(to_string(i) + ": track = "
                                    + to_string(radians_to_degrees(normalize_angle_difference(track.angles[i])))
                                    + "; process = "
                                    + to_string(radians_to_degrees(normalize_angle_difference(process_filter.angle())))
                                    + "; speed = "
                                    + to_string(radians_to_degrees(
                                            normalize_angle_difference(process_filter.angle_speed()))));
                        }
                        else
                        {
                                process_filter.update_acceleration(track.process_measurements[i].acceleration);
                                estimation_result.push_back(std::nullopt);
                        }
                }
                else
                {
                        process_filter.update_acceleration(track.process_measurements[i].acceleration);
                }

                process_result.push_back(process_filter.position());

                position_nees_average.add(track.positions[i], process_filter.position(), process_filter.position_p());
                angle_nees_average.add(track.angles[i], process_filter.angle(), process_filter.angle_p());
        }

        write_to_file(
                track.positions, angle_measurements(track, /*offset=*/T{-600}),
                acceleration_measurements(track, /*index=*/0, /*offset=*/T{-700}),
                acceleration_measurements(track, /*index=*/1, /*offset=*/T{-800}), position_measurements(track),
                speed_measurements(track, /*offset=*/T{-400}), estimation_result, process_result);

        LOG("Estimation Filter: " + estimation_nees_average.check_string());
        LOG("Position Filter: " + position_nees_average.check_string());
        LOG("Angle Filter: " + angle_nees_average.check_string());
}

void test()
{
        LOG("Test Filter 2D Extended");
        LOG("---");
        test_impl<float>();
        LOG("---");
        test_impl<double>();
        LOG("---");
        test_impl<long double>();
        LOG("---");
        LOG("Test Filter 2D Extended passed");
}

TEST_SMALL("Filter 2D Extended", test)
}
}
