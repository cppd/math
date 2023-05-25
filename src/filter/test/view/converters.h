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

#include <optional>
#include <vector>

namespace ns::filter::test::view
{
namespace converters_implementation
{
template <typename T>
T speed_kph(const T speed_mps)
{
        return T{3.6L} * speed_mps;
}

template <typename T>
T time_unit(const T time)
{
        return 10 * time;
}
}

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
        for (const SimulatorPoint<2, T>& p : track.points)
        {
                res.emplace_back(p.position);
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> track_speed(const Track<2, T>& track)
{
        namespace impl = converters_implementation;

        std::vector<Vector<2, T>> res;
        res.reserve(track.points.size());
        for (const SimulatorPoint<2, T>& p : track.points)
        {
                res.emplace_back(impl::time_unit(p.time), impl::speed_kph(p.speed));
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> position_measurements(const Track<2, T>& track, const T interval)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(track.position_measurements.size());
        std::optional<T> last_time;
        for (const PositionMeasurement<2, T>& m : track.position_measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (last_time && m.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(m.position);
                last_time = m.time;
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> speed_measurements(const Track<2, T>& track, const T interval)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(track.position_measurements.size());
        std::optional<T> last_time;
        for (const PositionMeasurement<2, T>& m : track.position_measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.speed)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(Vector<2, T>(impl::time_unit(m.time), impl::speed_kph(*m.speed)));
                last_time = m.time;
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> angle_measurements(const Track<2, T>& track)
{
        namespace impl = converters_implementation;

        std::vector<Vector<2, T>> res;
        res.reserve(track.process_measurements.size());
        std::optional<T> previous_angle;
        for (const ProcessMeasurement<2, T>& m : track.process_measurements)
        {
                const T angle = m.direction;
                const T unbounded_angle = unbound_angle(previous_angle, angle);
                previous_angle = unbounded_angle;
                res.emplace_back(impl::time_unit(m.time), radians_to_degrees(unbounded_angle));
        }
        return res;
}

template <std::size_t INDEX, typename T>
std::vector<Vector<2, T>> acceleration_measurements(const Track<2, T>& track)
{
        namespace impl = converters_implementation;

        static_assert(INDEX < 2);
        std::vector<Vector<2, T>> res;
        res.reserve(track.process_measurements.size());
        for (const ProcessMeasurement<2, T>& m : track.process_measurements)
        {
                res.emplace_back(impl::time_unit(m.time), m.acceleration[INDEX]);
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> optional_position(const std::vector<Vector<3, T>>& data, const T interval)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(data.size());
        std::optional<T> last_time;
        for (const Vector<3, T>& d : data)
        {
                ASSERT(!last_time || *last_time < d[0]);
                if (last_time && d[0] > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(Vector<2, T>(d[1], d[2]));
                last_time = d[0];
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> optional_speed(const std::vector<Vector<2, T>>& data, const T interval)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(data.size());
        std::optional<T> last_time;
        for (const Vector<2, T>& d : data)
        {
                ASSERT(!last_time || *last_time < d[0]);
                if (last_time && d[0] > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(d);
                last_time = d[0];
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> convert_speed(const std::vector<std::optional<Vector<2, T>>>& speed)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(speed.size());
        for (const std::optional<Vector<2, T>>& s : speed)
        {
                if (s)
                {
                        res.push_back(Vector<2, T>(impl::time_unit((*s)[0]), impl::speed_kph((*s)[1])));
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}
}
