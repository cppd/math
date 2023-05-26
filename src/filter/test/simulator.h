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

#pragma once

#include <src/numerical/vector.h>

#include <optional>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
struct SimulatorPoint final
{
        T time;
        Vector<N, T> position;
        T speed;
        T angle;
        T angle_r;
};

template <std::size_t N, typename T>
struct ProcessMeasurement final
{
        std::size_t simulator_point_index;
        T time;
        T direction;
        Vector<N, T> acceleration;
};

template <std::size_t N, typename T>
struct PositionMeasurement final
{
        std::size_t simulator_point_index;
        T time;
        Vector<N, T> position;
        std::optional<T> speed;
};

template <std::size_t N, typename T>
struct Track final
{
        std::vector<SimulatorPoint<N, T>> points;
        std::vector<ProcessMeasurement<N, T>> process_measurements;
        std::vector<PositionMeasurement<N, T>> position_measurements;
};

template <typename T>
struct TrackInfo final
{
        T dt;
        T speed_min;
        T speed_max;
        T speed_variance;
        T direction_bias_drift;
        T direction_angle;
};

template <typename T>
struct TrackMeasurementVariance final
{
        T direction;
        T acceleration;
        T position;
        T speed;
};

template <std::size_t N, typename T>
Track<N, T> generate_track(
        std::size_t count,
        const TrackInfo<T>& info,
        const TrackMeasurementVariance<T>& measurement_variance);
}
