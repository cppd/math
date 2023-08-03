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

#include "../measurement.h"
#include "../point.h"

#include <src/com/angle.h>
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
std::vector<Vector<2, T>> track_position(const std::vector<Measurements<2, T>>& measurements)
{
        std::vector<Vector<2, T>> res;
        res.reserve(measurements.size());
        for (const Measurements<2, T>& m : measurements)
        {
                res.emplace_back(m.true_data.position);
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> track_speed(const std::vector<Measurements<2, T>>& measurements)
{
        namespace impl = converters_implementation;

        std::vector<Vector<2, T>> res;
        res.reserve(measurements.size());
        for (const Measurements<2, T>& m : measurements)
        {
                res.emplace_back(impl::time_unit(m.time), mps_to_kph(m.true_data.speed));
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> position_measurements(
        const std::vector<Measurements<2, T>>& measurements,
        const T interval)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const Measurements<2, T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.position)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(m.position->value);
                last_time = m.time;
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> speed_measurements(
        const std::vector<Measurements<2, T>>& measurements,
        const T interval)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const Measurements<2, T>& m : measurements)
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
                res.push_back(Vector<2, T>(impl::time_unit(m.time), mps_to_kph(m.speed->value)));
                last_time = m.time;
        }
        return res;
}

template <typename T>
std::vector<Vector<2, T>> angle_measurements(const std::vector<Measurements<2, T>>& measurements, const T interval)
{
        namespace impl = converters_implementation;

        std::vector<Vector<2, T>> res;
        res.reserve(measurements.size());
        std::optional<T> previous_angle;
        std::optional<T> last_time;
        for (const Measurements<2, T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.direction)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                const T unbounded_angle = unbound_angle(previous_angle, m.direction->value);
                previous_angle = unbounded_angle;
                res.emplace_back(impl::time_unit(m.time), radians_to_degrees(unbounded_angle));
                last_time = m.time;
        }
        return res;
}

template <std::size_t INDEX, typename T>
std::vector<Vector<2, T>> acceleration_measurements(
        const std::vector<Measurements<2, T>>& measurements,
        const T interval)
{
        namespace impl = converters_implementation;

        static_assert(INDEX < 2);
        std::vector<Vector<2, T>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const Measurements<2, T>& m : measurements)
        {
                ASSERT(!last_time || *last_time < m.time);
                if (!m.acceleration)
                {
                        continue;
                }
                if (last_time && m.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.emplace_back(impl::time_unit(m.time), m.acceleration->value[INDEX]);
                last_time = m.time;
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<Point<N, T>>> optional_value(const std::vector<Point<N, T>>& points, const T interval)
{
        std::vector<std::optional<Point<N, T>>> res;
        res.reserve(points.size());
        std::optional<T> last_time;
        for (const Point<N, T>& p : points)
        {
                ASSERT(!last_time || *last_time < p.time);
                if (last_time && p.time > *last_time + interval)
                {
                        res.emplace_back();
                }
                res.push_back(p);
                last_time = p.time;
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> convert_position(const std::vector<std::optional<Point<2, T>>>& position)
{
        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(position.size());
        for (const std::optional<Point<2, T>>& p : position)
        {
                if (p)
                {
                        res.push_back(p->point);
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> convert_speed(const std::vector<std::optional<Point<1, T>>>& speed)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(speed.size());
        for (const std::optional<Point<1, T>>& s : speed)
        {
                if (s)
                {
                        res.push_back({
                                {impl::time_unit(s->time), mps_to_kph(s->point)}
                        });
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <typename T>
std::vector<std::optional<Vector<2, T>>> convert_speed_p(const std::vector<std::optional<Point<1, T>>>& speed_p)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(speed_p.size());
        for (const std::optional<Point<1, T>>& s : speed_p)
        {
                if (s && !std::isnan(std::sqrt(s->point)))
                {
                        res.push_back({
                                {impl::time_unit(s->time), mps_to_kph(std::sqrt(s->point))}
                        });
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <std::size_t INDEX, typename T>
std::vector<std::optional<Vector<2, T>>> convert_position_p(const std::vector<std::optional<Point<2, T>>>& position_p)
{
        namespace impl = converters_implementation;
        static_assert(INDEX < 2);

        std::vector<std::optional<Vector<2, T>>> res;
        res.reserve(position_p.size());
        for (const std::optional<Point<2, T>>& p : position_p)
        {
                if (p && !std::isnan(std::sqrt(p->point[INDEX])))
                {
                        res.push_back({
                                {impl::time_unit(p->time), std::sqrt(p->point[INDEX])}
                        });
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}
}
