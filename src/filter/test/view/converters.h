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

#include "../simulator.h"
#include "../utility.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <map>
#include <optional>
#include <vector>

namespace ns::filter::test::view
{
template <typename T>
std::vector<std::optional<Vector<2, T>>> add_offset(
        const std::vector<std::optional<Vector<2, T>>>& data,
        const std::type_identity_t<T> offset)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(data.size());
        for (const auto& d : data)
        {
                if (d)
                {
                        res.push_back(Vector<2, T>((*d)[0], (*d)[1] + offset));
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> add_offset(const std::vector<Vector<2, T>>& data, const std::type_identity_t<T> offset)
{
        std::vector<Vector<2, T>> res;
        res.reserve(data.size());
        for (const auto& d : data)
        {
                res.emplace_back(d[0], d[1] + offset);
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> track_position(const Track<2, T>& track)
{
        std::vector<Vector<2, T>> res;
        res.reserve(track.points.size());
        for (std::size_t i = 0; i < track.points.size(); ++i)
        {
                res.emplace_back(track.points[i].position);
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> track_speed(const Track<2, T>& track)
{
        std::vector<Vector<2, T>> res;
        res.reserve(track.points.size());
        for (std::size_t i = 0; i < track.points.size(); ++i)
        {
                res.emplace_back(track.points[i].position[0], track.points[i].speed);
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> position_measurements(const Track<2, T>& track)
{
        std::vector<std::optional<Vector<2, T>>> res;
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
std::vector<std::optional<Vector<2, T>>> speed_measurements(const Track<2, T>& track)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(track.position_measurements.size());
        for (const auto& [i, v] : std::map{track.position_measurements.cbegin(), track.position_measurements.cend()})
        {
                if (v && v->speed)
                {
                        res.push_back(Vector<2, T>(track.points[i].position[0], *v->speed));
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> angle_measurements(const Track<2, T>& track)
{
        std::vector<Vector<2, T>> res;
        res.reserve(track.points.size());
        std::optional<T> previous_angle;
        for (std::size_t i = 0; i < track.points.size(); ++i)
        {
                const T vx = track.process_measurements[i].direction[0];
                const T vy = track.process_measurements[i].direction[1];
                const T angle = std::atan2(vy, vx);
                const T unbounded_angle = unbound_angle(previous_angle, angle);
                previous_angle = unbounded_angle;
                res.emplace_back(track.points[i].position[0], radians_to_degrees(unbounded_angle));
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> acceleration_measurements(const Track<2, T>& track, const std::size_t index)
{
        ASSERT(index < 2);
        std::vector<Vector<2, T>> res;
        res.reserve(track.points.size());
        for (std::size_t i = 0; i < track.points.size(); ++i)
        {
                res.emplace_back(track.points[i].position[0], track.process_measurements[i].acceleration[index]);
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> filter_speed(
        const Track<2, T>& track,
        const std::vector<std::optional<T>>& speed)
{
        ASSERT(track.points.size() == speed.size());
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(track.points.size());
        for (std::size_t i = 0; i < track.points.size(); ++i)
        {
                if (speed[i])
                {
                        res.push_back(Vector<2, T>(track.points[i].position[0], *speed[i]));
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}
}
