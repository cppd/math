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
#include "../models.h"

#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <array>
#include <vector>

namespace ns::filter::test
{
namespace
{
template <typename T>
void test_impl()
{
        constexpr std::size_t N = 4;
        constexpr std::size_t M = 2;

        constexpr T DT = 1;
        constexpr T TRACK_VELOCITY_MEAN = 1;
        constexpr T TRACK_VELOCITY_VARIANCE = power<2>(0.1);
        constexpr T PROCESS_VARIANCE = power<2>(0.1);

        constexpr T VELOCITY_MEASUREMENT_VARIANCE = power<2>(0.2);
        constexpr T POSITION_MEASUREMENT_VARIANCE = power<2>(3);

        constexpr Vector<N, T> X(10, 5, 10, 5);
        constexpr Matrix<N, N, T> P{
                {500,  0,   0,  0},
                {  0, 50,   0,  0},
                {  0,  0, 500,  0},
                {  0,  0,   0, 50}
        };
        constexpr Matrix<N, N, T> F{
                {1, DT, 0,  0},
                {0,  1, 0,  0},
                {0,  0, 1, DT},
                {0,  0, 0,  1}
        };
        constexpr Matrix<N, N, T> Q = []()
        {
                const auto m = discrete_white_noise<N / 2, T>(DT, PROCESS_VARIANCE);
                return block_diagonal(std::array{m, m});
        }();

        constexpr Matrix<M, N, T> POSITION_H{
                {1, 0, 0, 0},
                {0, 0, 1, 0}
        };
        constexpr Matrix<N, M, T> POSITION_H_T = POSITION_H.transpose();
        constexpr Matrix<M, M, T> POSITION_R{
                {POSITION_MEASUREMENT_VARIANCE,                             0},
                {                            0, POSITION_MEASUREMENT_VARIANCE}
        };

        constexpr Matrix<M, N, T> VELOCITY_H{
                {0, 1, 0, 0},
                {0, 0, 0, 1}
        };
        constexpr Matrix<N, M, T> VELOCITY_H_T = VELOCITY_H.transpose();
        constexpr Matrix<M, M, T> VELOCITY_R{
                {VELOCITY_MEASUREMENT_VARIANCE,                             0},
                {                            0, VELOCITY_MEASUREMENT_VARIANCE}
        };

        constexpr std::size_t COUNT = 1000;
        constexpr std::array<std::size_t, 2> POSITION_OUTAGE = {350, 400};
        constexpr std::size_t POSITION_INTERVAL = 5;

        const Track track = generate_track<2, T>(
                COUNT, DT, TRACK_VELOCITY_MEAN, TRACK_VELOCITY_VARIANCE, VELOCITY_MEASUREMENT_VARIANCE,
                POSITION_MEASUREMENT_VARIANCE, POSITION_OUTAGE, POSITION_INTERVAL);

        Filter<N, T> filter;
        filter.set_x(X);
        filter.set_p(P);
        filter.set_f(F);
        filter.set_q(Q);

        std::vector<Vector<2, T>> result;
        result.reserve(COUNT);
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                filter.predict();

                if (const auto iter = track.position_measurements.find(i);
                    iter != track.position_measurements.cend() && iter->second)
                {
                        filter.update(POSITION_H, POSITION_H_T, POSITION_R, *iter->second);
                }
                else
                {
                        filter.update(VELOCITY_H, VELOCITY_H_T, VELOCITY_R, track.velocity_measurements[i]);
                }

                result.push_back({filter.x()[0], filter.x()[2]});
        }

        write_to_file(track.positions, track.position_measurements, result);
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
