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
#include "utility.h"

#include "view/write.h"

#include "../ekf.h"
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
        static constexpr T POSITION_DT = POSITION_INTERVAL * DT;

        static constexpr T TRACK_VELOCITY_MEAN = 20;
        static constexpr T TRACK_VELOCITY_VARIANCE = square(0.1);

        static constexpr T DIRECTION_BIAS_DRIFT = degrees_to_radians(360.0);
        static constexpr T DIRECTION_ANGLE = degrees_to_radians(10.0);

        static constexpr T MEASUREMENT_DIRECTION_VARIANCE = square(degrees_to_radians(2.0));
        static constexpr T MEASUREMENT_ACCELERATION_VARIANCE = square(1.0);
        static constexpr T MEASUREMENT_POSITION_VARIANCE = square(20.0);
        static constexpr T MEASUREMENT_POSITION_SPEED_VARIANCE = square(0.2);

        static constexpr T POSITION_FILTER_VARIANCE = square(0.2);

        static constexpr T PROCESS_POSITION_VARIANCE = square(0.2);
        static constexpr T PROCESS_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
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
        oss << "speed: " << std::sqrt(Config<T>::MEASUREMENT_POSITION_SPEED_VARIANCE) << " m/s";
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
                .position_speed = Config<T>::MEASUREMENT_POSITION_SPEED_VARIANCE};

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

        static Matrix<N, N, T> f(const T dt)
        {
                const T dt_2 = square(dt) / 2;
                return Matrix<N, N, T>{
                        {1, dt, dt_2, 0,  0,    0},
                        {0,  1,   dt, 0,  0,    0},
                        {0,  0,    1, 0,  0,    0},
                        {0,  0,    0, 1, dt, dt_2},
                        {0,  0,    0, 0,  1,   dt},
                        {0,  0,    0, 0,  0,    1}
                };
        }

        static Matrix<N, N, T> q(const T dt)
        {
                const T dt_2 = square(dt) / 2;
                const Matrix<N, 2, T> noise_transition{
                        {dt_2,    0},
                        {  dt,    0},
                        {   1,    0},
                        {   0, dt_2},
                        {   0,   dt},
                        {   0,    1},
                };

                const T p = Config<T>::POSITION_FILTER_VARIANCE;
                const Matrix<2, 2, T> process_covariance{
                        {p, 0},
                        {0, p}
                };

                return noise_transition * process_covariance * noise_transition.transposed();
        }

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

        Ekf<N, T> filter_;

public:
        PositionFilter(const Vector<N, T> init_x, const Matrix<N, N, T>& init_p)
                : filter_(init_x, init_p)
        {
        }

        void predict(const T dt)
        {
                ASSERT(dt >= 0);
                const auto& f_matrix = f(dt);
                const auto& q_matrix = q(dt);
                filter_.predict(f_matrix, f_matrix.transposed(), q_matrix);
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
        static constexpr std::size_t N = 9;

        static constexpr Matrix<N, N, T> F = []()
        {
                const T dt = Config<T>::DT;
                const T dt_2 = square(dt) / 2;
                return Matrix<N, N, T>{
                        {1, dt, dt_2, 0,  0,    0, 0,  0, 0},
                        {0,  1,   dt, 0,  0,    0, 0,  0, 0},
                        {0,  0,    1, 0,  0,    0, 0,  0, 0},
                        {0,  0,    0, 1, dt, dt_2, 0,  0, 0},
                        {0,  0,    0, 0,  1,   dt, 0,  0, 0},
                        {0,  0,    0, 0,  0,    1, 0,  0, 0},
                        {0,  0,    0, 0,  0,    0, 1, dt, 0},
                        {0,  0,    0, 0,  0,    0, 0,  1, 0},
                        {0,  0,    0, 0,  0,    0, 0,  0, 1}
                };
        }();

        static constexpr Matrix<N, N, T> F_T = F.transposed();

        static constexpr Matrix<N, N, T> Q = []()
        {
                const T dt = Config<T>::DT;
                const T dt_2 = square(dt) / 2;
                const Matrix<N, 4, T> noise_transition{
                        {dt_2,    0,    0, 0},
                        {  dt,    0,    0, 0},
                        {   1,    0,    0, 0},
                        {   0, dt_2,    0, 0},
                        {   0,   dt,    0, 0},
                        {   0,    1,    0, 0},
                        {   0,    0, dt_2, 0},
                        {   0,    0,   dt, 0},
                        {   0,    0,    0, 1}
                };

                const T p = Config<T>::PROCESS_POSITION_VARIANCE;
                const T a = Config<T>::PROCESS_ANGLE_VARIANCE;
                const T a_r = Config<T>::PROCESS_ANGLE_R_VARIANCE;
                const Matrix<4, 4, T> process_covariance{
                        {p, 0, 0,   0},
                        {0, p, 0,   0},
                        {0, 0, a,   0},
                        {0, 0, 0, a_r}
                };

                return noise_transition * process_covariance * noise_transition.transposed();
        }();

        static constexpr Matrix<2, N, T> POSITION_H{
                {1, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0, 0, 0, 0}
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
                // dx = vx*cos(angle + angle_r) - vy*sin(angle + angle_r)
                // dy = vx*sin(angle + angle_r) + vy*cos(angle + angle_r)
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                const T px = x[0];
                const T vx = x[1];
                const T ax = x[2];
                const T py = x[3];
                const T vy = x[4];
                const T ay = x[5];
                const T angle = x[6];
                const T angle_r = x[8];
                const T cos_v = std::cos(angle + angle_r);
                const T sin_v = std::sin(angle + angle_r);
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {px,
                        py,
                        vx * cos_v - vy * sin_v,
                        vx * sin_v + vy * cos_v,
                        ax * cos - ay * sin,
                        ax * sin + ay * cos};
        }

        static Matrix<6, N, T> position_velocity_acceleration_hj(const Vector<N, T>& x)
        {
                // x = px
                // y = py
                // dx = vx*cos(angle + angle_r) - vy*sin(angle + angle_r)
                // dy = vx*sin(angle + angle_r) + vy*cos(angle + angle_r)
                // ax = ax*cos(angle) - ay*sin(angle)
                // ay = ax*sin(angle) + ay*cos(angle)
                // Jacobian
                const T vx = x[1];
                const T vy = x[4];
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T angle_r = x[8];
                const T cos_v = std::cos(angle + angle_r);
                const T sin_v = std::sin(angle + angle_r);
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                const T d_1 = -vx * sin_v - vy * cos_v;
                const T d_2 = vx * cos_v - vy * sin_v;
                const T a_1 = -ax * sin - ay * cos;
                const T a_2 = ax * cos - ay * sin;
                return {
                        {1,     0,   0, 0,      0,    0,   0, 0,   0},
                        {0,     0,   0, 1,      0,    0,   0, 0,   0},
                        {0, cos_v,   0, 0, -sin_v,    0, d_1, 0, d_1},
                        {0, sin_v,   0, 0,  cos_v,    0, d_2, 0, d_2},
                        {0,     0, cos, 0,      0, -sin, a_1, 0,   0},
                        {0,     0, sin, 0,      0,  cos, a_2, 0,   0}
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
                        {0, 0, cos, 0, 0, -sin, -ax * sin - ay * cos, 0, 0},
                        {0, 0, sin, 0, 0,  cos,  ax * cos - ay * sin, 0, 0}
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
                // dx = (vx*cos(angle + angle_r) - vy*sin(angle + angle_r)) / sqrt(vx*vx + vy*vy);
                // dy = (vx*sin(angle + angle_r) + vy*cos(angle + angle_r)) / sqrt(vx*vx + vy*vy);
                // ax = (ax*cos(angle) - ay*sin(angle))
                // ay = (ax*sin(angle) + ay*cos(angle))
                const T px = x[0];
                const T vx = x[1];
                const T ax = x[2];
                const T py = x[3];
                const T vy = x[4];
                const T ay = x[5];
                const T angle = x[6];
                const T angle_r = x[8];
                const T speed = std::sqrt(square(vx) + square(vy));
                const T cos_v = std::cos(angle + angle_r);
                const T sin_v = std::sin(angle + angle_r);
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                return {px,
                        py,
                        (vx * cos_v - vy * sin_v) / speed,
                        (vx * sin_v + vy * cos_v) / speed,
                        ax * cos - ay * sin,
                        ax * sin + ay * cos};
        }

        static Matrix<6, N, T> position_direction_acceleration_hj(const Vector<N, T>& x)
        {
                // px = px
                // py = py
                // dx = (vx*cos(angle + angle_r) - vy*sin(angle + angle_r)) / sqrt(vx*vx + vy*vy);
                // dy = (vx*sin(angle + angle_r) + vy*cos(angle + angle_r)) / sqrt(vx*vx + vy*vy);
                // ax = (ax*cos(angle) - ay*sin(angle))
                // ay = (ax*sin(angle) + ay*cos(angle))
                // Jacobian
                // mPx=Px;
                // mPy=Py;
                // mDx=(Vx*Cos[Angle+AngleR]-Vy*Sin[Angle+AngleR])/Sqrt[Vx*Vx+Vy*Vy];
                // mDy=(Vx*Sin[Angle+AngleR]+Vy*Cos[Angle+AngleR])/Sqrt[Vx*Vx+Vy*Vy];
                // mAx=(Ax*Cos[Angle]-Ay*Sin[Angle]);
                // mAy=(Ax*Sin[Angle]+Ay*Cos[Angle]);
                // Simplify[D[{mPx,mPy,mDx,mDy,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Angle,AngleV,AngleR}}]]
                const T vx = x[1];
                const T vy = x[4];
                const T ax = x[2];
                const T ay = x[5];
                const T angle = x[6];
                const T angle_r = x[8];
                const T l = std::sqrt(square(vx) + square(vy));
                const T l_3 = power<3>(l);
                const T cos_v = std::cos(angle + angle_r);
                const T sin_v = std::sin(angle + angle_r);
                const T cos = std::cos(angle);
                const T sin = std::sin(angle);
                const T d_1 = vy * cos_v + vx * sin_v;
                const T d_2 = vx * cos_v - vy * sin_v;
                const T a_1 = -ax * sin - ay * cos;
                const T a_2 = ax * cos - ay * sin;
                return {
                        {1,               0,   0, 0,               0,    0,        0, 0,        0},
                        {0,               0,   0, 1,               0,    0,        0, 0,        0},
                        {0,  vy * d_1 / l_3,   0, 0, -vx * d_1 / l_3,    0, -d_1 / l, 0, -d_1 / l},
                        {0, -vy * d_2 / l_3,   0, 0,  vx * d_2 / l_3,    0,  d_2 / l, 0,  d_2 / l},
                        {0,               0, cos, 0,               0, -sin,      a_1, 0,        0},
                        {0,               0, sin, 0,               0,  cos,      a_2, 0,        0}
                };
        }

        Ekf<N, T> filter_;

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

        [[nodiscard]] T speed() const
        {
                return Vector<2, T>(filter_.x()[1], filter_.x()[4]).norm();
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

        [[nodiscard]] T angle_r() const
        {
                return filter_.x()[8];
        }

        [[nodiscard]] T angle_r_p() const
        {
                return filter_.p()(8, 8);
        }
};

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

        PositionFilter<T> position_filter(position_init_x, position_init_p);

        std::vector<std::optional<Vector<2, T>>> result_position;
        result_position.reserve(track.points.size());

        NeesAverage<2, T> position_nees_average;

        std::optional<std::size_t> last_position_i;
        auto position_iter = track.position_measurements.cbegin();

        for (; position_iter != track.position_measurements.cend(); ++position_iter)
        {
                const PositionMeasurement<2, T>& m = *position_iter;

                ASSERT(!last_position_i || last_position_i < m.index);

                if (last_position_i && m.index > *last_position_i + Config<T>::POSITION_INTERVAL)
                {
                        result_position.emplace_back();
                }

                if (!last_position_i)
                {
                        last_position_i = m.index;
                }

                position_filter.predict((m.index - *last_position_i) * Config<T>::DT);
                position_filter.update(m.position);
                last_position_i = m.index;

                result_position.push_back(position_filter.position());

                position_nees_average.add(
                        track.points[m.index].position, position_filter.position(), position_filter.position_p());

                if (m.index >= 300)
                {
                        break;
                }
        }

        ASSERT(last_position_i && position_iter != track.position_measurements.cend());
        ASSERT(position_iter->index == *last_position_i);

        const Angle<T> angle = position_filter.velocity_angle();

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

        ProcessFilter<T> process_filter(init_x, init_p);

        std::vector<Vector<2, T>> result_process;
        result_process.reserve(track.points.size());

        std::vector<std::optional<T>> result_speed;
        result_speed.reserve(track.points.size());
        result_speed.resize(*last_position_i);

        NeesAverage<2, T> process_position_nees_average;
        NeesAverage<1, T> process_angle_nees_average;
        NeesAverage<1, T> process_angle_r_nees_average;

        ++position_iter;
        ASSERT(position_iter->index > *last_position_i);

        for (std::size_t i = *last_position_i; i < track.points.size(); ++i)
        {
                process_filter.predict();

                if (position_iter != track.position_measurements.cend() && position_iter->index == i)
                {
                        const auto& measurement = *position_iter;

                        if (i > *last_position_i + Config<T>::POSITION_INTERVAL)
                        {
                                result_position.emplace_back();
                        }

                        position_filter.predict((i - *last_position_i) * Config<T>::DT);
                        position_filter.update(measurement.position);

                        result_position.push_back(position_filter.position());

                        position_nees_average.add(
                                track.points[i].position, position_filter.position(), position_filter.position_p());

                        if (measurement.speed)
                        {
                                process_filter.update_position_velocity_acceleration(
                                        measurement.position, track.process_measurements[i].direction,
                                        *measurement.speed, track.process_measurements[i].acceleration);
                        }
                        else
                        {
                                process_filter.update_position_direction_acceleration(
                                        measurement.position, track.process_measurements[i].direction,
                                        track.process_measurements[i].acceleration);
                        }

                        LOG(to_string(i) + ": track = "
                            + to_string(radians_to_degrees(normalize_angle_difference(track.points[i].angle)))
                            + "; process = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_filter.angle())))
                            + "; speed = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_filter.angle_speed())))
                            + "; r = "
                            + to_string(radians_to_degrees(normalize_angle_difference(process_filter.angle_r()))));

                        ASSERT(i == position_iter->index);
                        last_position_i = position_iter->index;
                        ++position_iter;
                        ASSERT(position_iter == track.position_measurements.cend()
                               || position_iter->index > *last_position_i);
                }
                else
                {
                        process_filter.update_acceleration(track.process_measurements[i].acceleration);
                }

                result_process.push_back(process_filter.position());
                result_speed.push_back(process_filter.speed());

                process_position_nees_average.add(
                        track.points[i].position, process_filter.position(), process_filter.position_p());
                process_angle_nees_average.add(track.points[i].angle, process_filter.angle(), process_filter.angle_p());
                process_angle_r_nees_average.add(
                        track.points[i].angle_r, process_filter.angle_r(), process_filter.angle_r_p());
        }

        view::write_to_file(
                make_annotation<T>(), track, Config<T>::POSITION_INTERVAL, result_position, result_speed,
                result_process);

        LOG("Position Filter: " + position_nees_average.check_string());
        LOG("Process Filter: " + process_position_nees_average.check_string());
        LOG("Angle Filter: " + process_angle_nees_average.check_string());
        LOG("Angle R Filter: " + process_angle_r_nees_average.check_string());
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
