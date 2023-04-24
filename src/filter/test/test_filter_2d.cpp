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
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <array>
#include <map>
#include <vector>

namespace ns::filter::test
{
namespace
{
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
void check_nees(
        const std::vector<Vector<N, T>>& values,
        const std::vector<Vector<N, T>>& estimates,
        const std::vector<Matrix<N, N, T>>& covariances)
{
        const T nees = nees_average(values, estimates, covariances);

        LOG(std::string("NEES average <") + type_name<T>() + "> = " + to_string(nees) + "; " + to_string(N) + " degree"
            + (N > 1 ? "s" : "") + " of freedom; " + (nees <= N ? " passed" : "failed"));
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
Matrix<2, 2, T> measurement_r(const Matrix<2, 2, T>& velocity_r, const Vector<2, T>& velocity)
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
        return error_propagation * velocity_r * error_propagation.transposed();
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
void test_impl()
{
        constexpr std::size_t N = 4;
        constexpr std::size_t M = 2;

        constexpr T DT = 0.1;

        constexpr T TRACK_VELOCITY_MEAN = 10;
        constexpr T TRACK_VELOCITY_VARIANCE = power<2>(1);
        constexpr T PROCESS_VARIANCE = TRACK_VELOCITY_VARIANCE;

        constexpr T VELOCITY_AMOUNT_MEASUREMENT_VARIANCE = power<2>(1);
        constexpr T VELOCITY_DIRECTION_MEASUREMENT_VARIANCE = power<2>(0.1);

        constexpr T POSITION_MEASUREMENT_VARIANCE = power<2>(30);
        constexpr std::size_t POSITION_INTERVAL = 10;

        constexpr Vector<N, T> X(10, 0, 10, 5);
        constexpr Matrix<N, N, T> P{
                {500,   0,   0,   0},
                {  0, 100,   0,   0},
                {  0,   0, 500,   0},
                {  0,   0,   0, 100}
        };

        constexpr Matrix<N, N, T> F{
                {1, DT, 0,  0},
                {0,  1, 0,  0},
                {0,  0, 1, DT},
                {0,  0, 0,  1}
        };
        constexpr Matrix<N, N, T> F_T = F.transposed();
        constexpr Matrix<N, N, T> Q = []()
        {
                constexpr Matrix<N, 2, T> NOISE_TRANSITION{
                        {DT * DT / 2,           0},
                        {         DT,           0},
                        {          0, DT * DT / 2},
                        {          0,          DT}
                };
                constexpr Matrix<2, 2, T> PROCESS_COVARIANCE{
                        {PROCESS_VARIANCE,                0},
                        {               0, PROCESS_VARIANCE}
                };
                return NOISE_TRANSITION * PROCESS_COVARIANCE * NOISE_TRANSITION.transposed();
        }();

        constexpr Matrix<M, N, T> VELOCITY_H{
                {0, 1, 0, 0},
                {0, 0, 0, 1}
        };
        constexpr Matrix<N, M, T> VELOCITY_H_T = VELOCITY_H.transposed();
        constexpr Matrix<M, M, T> VELOCITY_MEASUREMENT_R{
                {VELOCITY_AMOUNT_MEASUREMENT_VARIANCE,                                       0},
                {                                   0, VELOCITY_DIRECTION_MEASUREMENT_VARIANCE}
        };

        constexpr Matrix<M, N, T> POSITION_H{
                {1, 0, 0, 0},
                {0, 0, 1, 0}
        };
        constexpr Matrix<N, M, T> POSITION_H_T = POSITION_H.transposed();
        constexpr Matrix<M, M, T> POSITION_R = {
                {POSITION_MEASUREMENT_VARIANCE,                             0},
                {                            0, POSITION_MEASUREMENT_VARIANCE}
        };

        const Track track = [&]()
        {
                constexpr std::size_t COUNT = 3000;

                Track res = generate_track<2, T>(
                        COUNT, DT, TRACK_VELOCITY_MEAN, TRACK_VELOCITY_VARIANCE, VELOCITY_AMOUNT_MEASUREMENT_VARIANCE,
                        VELOCITY_DIRECTION_MEASUREMENT_VARIANCE, POSITION_MEASUREMENT_VARIANCE, POSITION_INTERVAL);
                for (auto& [i, p] : res.position_measurements)
                {
                        ASSERT(i >= 0 && i < COUNT);
                        if (i >= 1100 && i <= 1600)
                        {
                                p.reset();
                        }
                }
                return res;
        }();

        Filter<N, T> filter(X, P);

        std::vector<Vector<2, T>> result;
        result.reserve(track.positions.size());
        std::vector<Matrix<2, 2, T>> result_p;
        result_p.reserve(track.positions.size());
        for (std::size_t i = 0; i < track.positions.size(); ++i)
        {
                filter.predict(F, F_T, Q);

                if (const auto iter = track.position_measurements.find(i);
                    iter != track.position_measurements.cend() && iter->second)
                {
                        filter.update(POSITION_H, POSITION_H_T, POSITION_R, iter->second->position);

                        {
                                const Matrix<M, M, T> velocity_p{
                                        {filter.p()(1, 1), filter.p()(1, 3)},
                                        {filter.p()(3, 1), filter.p()(3, 3)}
                                };
                                const Vector<M, T> velocity(filter.x()[1], filter.x()[3]);
                                const Matrix<M, M, T> r = measurement_r(velocity_p, velocity);
                                const T velocity_angle = std::atan2(velocity[1], velocity[0]);
                                const T measurement_angle = std::atan2(
                                        track.velocity_measurements[i].direction[1],
                                        track.velocity_measurements[i].direction[0]);
                                const T angle_difference =
                                        normalize_angle_difference(velocity_angle - measurement_angle);
                                LOG("i = " + to_string(i) + "; angle difference = " + to_string(angle_difference)
                                    + "; direction difference sigma = "
                                    + to_string(std::sqrt(r(1, 1) + VELOCITY_MEASUREMENT_R(1, 1))));
                        }
                }
                else
                {
                        const auto& direction = track.velocity_measurements[i].direction;
                        const auto& amount = track.velocity_measurements[i].amount;
                        const Vector<M, T> velocity = direction * amount;
                        const Matrix<M, M, T> r = velocity_r(VELOCITY_MEASUREMENT_R, direction, amount);
                        filter.update(VELOCITY_H, VELOCITY_H_T, r, velocity);
                }

                result.push_back({filter.x()[0], filter.x()[2]});

                result_p.push_back({
                        {filter.p()(0, 0), filter.p()(0, 2)},
                        {filter.p()(2, 0), filter.p()(2, 2)}
                });
        }

        write_to_file(track.positions, position_measurements(track), result);

        check_nees(track.positions, result, result_p);
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
