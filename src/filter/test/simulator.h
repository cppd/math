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
        std::optional<T> direction;
        std::optional<Vector<N, T>> acceleration;
        std::optional<Vector<N, T>> position;
        std::optional<T> speed;
};

template <std::size_t N, typename T>
struct Track final
{
        std::vector<SimulatorPoint<N, T>> points;
        std::vector<ProcessMeasurement<N, T>> measurements;
};

template <typename T>
struct TrackInfo final
{
        T track_speed_min;
        T track_speed_max;
        T track_speed_variance;
        T track_direction_bias_drift;
        T track_direction_angle;
        T measurement_dt;
        unsigned measurement_dt_count_acceleration;
        unsigned measurement_dt_count_direction;
        unsigned measurement_dt_count_position;
        unsigned measurement_dt_count_speed;
        T measurement_variance_acceleration;
        T measurement_variance_direction;
        T measurement_variance_position;
        T measurement_variance_speed;
};

template <std::size_t N, typename T>
Track<N, T> generate_track(std::size_t count, const TrackInfo<T>& info);
}
