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
#include <unordered_map>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
struct ProcessMeasurement final
{
        Vector<N, T> direction;
        T amount;
        Vector<N, T> acceleration;
};

template <std::size_t N, typename T>
struct PositionMeasurement final
{
        Vector<N, T> position;
        T speed;
};

template <std::size_t N, typename T>
struct Track final
{
        std::vector<Vector<N, T>> positions;
        std::vector<T> angles;
        std::vector<ProcessMeasurement<N, T>> process_measurements;
        std::unordered_map<std::size_t, std::optional<PositionMeasurement<N, T>>> position_measurements;
};

template <typename T>
struct TrackMeasurementVariance final
{
        T velocity_amount;
        T velocity_direction;
        T acceleration;
        T position;
        T position_speed;
};

template <std::size_t N, typename T>
Track<N, T> generate_track(
        std::size_t count,
        T dt,
        T track_velocity_mean,
        T track_velocity_variance,
        const TrackMeasurementVariance<T>& track_measurement_variance,
        std::size_t position_interval);
}
