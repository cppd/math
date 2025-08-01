/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "point.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/filter/filters/measurement.h>
#include <src/numerical/vector.h>

#include <cstddef>
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

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<N, T>>> add_offset(
        const std::vector<std::optional<numerical::Vector<N, T>>>& data,
        const numerical::Vector<N, T>& offset)
{
        std::vector<std::optional<numerical::Vector<N, T>>> res;
        res.reserve(data.size());
        for (const auto& d : data)
        {
                if (d)
                {
                        res.push_back(*d + offset);
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<numerical::Vector<N, T>> add_offset(
        const std::vector<numerical::Vector<N, T>>& data,
        const numerical::Vector<N, T>& offset)
{
        std::vector<numerical::Vector<N, T>> res;
        res.reserve(data.size());
        for (const auto& d : data)
        {
                res.push_back(d + offset);
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<numerical::Vector<N, T>> track_position(const std::vector<filters::Measurements<N, T>>& measurements)
{
        std::vector<numerical::Vector<N, T>> res;
        res.reserve(measurements.size());
        for (const filters::Measurements<N, T>& m : measurements)
        {
                res.emplace_back(m.true_data.position);
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<numerical::Vector<2, T>> track_speed(const std::vector<filters::Measurements<N, T>>& measurements)
{
        namespace impl = converters_implementation;

        std::vector<numerical::Vector<2, T>> res;
        res.reserve(measurements.size());
        for (const filters::Measurements<N, T>& m : measurements)
        {
                res.emplace_back(impl::time_unit(m.time), mps_to_kph(m.true_data.speed));
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<N, T>>> position_measurements(
        const std::vector<filters::Measurements<N, T>>& measurements,
        const T interval)
{
        std::vector<std::optional<numerical::Vector<N, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const filters::Measurements<N, T>& m : measurements)
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

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> speed_measurements(
        const std::vector<filters::Measurements<N, T>>& measurements,
        const T interval)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const filters::Measurements<N, T>& m : measurements)
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
                res.push_back({
                        {impl::time_unit(m.time), mps_to_kph(m.speed->value[0])}
                });
                last_time = m.time;
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> angle_measurements(
        const std::vector<filters::Measurements<N, T>>& measurements,
        const T interval)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> previous_angle;
        std::optional<T> last_time;
        for (const filters::Measurements<N, T>& m : measurements)
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
                const T unbounded_angle = unbound_angle(previous_angle, m.direction->value[0]);
                previous_angle = unbounded_angle;
                res.push_back({
                        {impl::time_unit(m.time), radians_to_degrees(unbounded_angle)}
                });
                last_time = m.time;
        }
        return res;
}

template <std::size_t INDEX, std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> acceleration_measurements(
        const std::vector<filters::Measurements<N, T>>& measurements,
        const T interval)
{
        static_assert(INDEX < N);

        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(measurements.size());
        std::optional<T> last_time;
        for (const filters::Measurements<N, T>& m : measurements)
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
                res.push_back({
                        {impl::time_unit(m.time), m.acceleration->value[INDEX]}
                });
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

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<N, T>>> convert_position(
        const std::vector<std::optional<Point<N, T>>>& points)
{
        std::vector<std::optional<numerical::Vector<N, T>>> res;
        res.reserve(points.size());
        for (const std::optional<Point<N, T>>& p : points)
        {
                if (p)
                {
                        res.push_back(p->position);
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> convert_speed(const std::vector<std::optional<Point<N, T>>>& points)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(points.size());
        for (const std::optional<Point<N, T>>& p : points)
        {
                if (p)
                {
                        res.push_back({
                                {impl::time_unit(p->time), mps_to_kph(p->speed)}
                        });
                }
                else
                {
                        res.emplace_back();
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> convert_speed_p(
        const std::vector<std::optional<Point<N, T>>>& points)
{
        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(points.size());
        for (const std::optional<Point<N, T>>& p : points)
        {
                if (p)
                {
                        const T sd = std::sqrt(p->speed_p);
                        if (!std::isnan(sd))
                        {
                                res.push_back({
                                        {impl::time_unit(p->time), mps_to_kph(sd)}
                                });
                                continue;
                        }
                }
                res.emplace_back();
        }
        return res;
}

template <std::size_t INDEX, std::size_t N, typename T>
std::vector<std::optional<numerical::Vector<2, T>>> convert_position_p(
        const std::vector<std::optional<Point<N, T>>>& points)
{
        static_assert(INDEX < N);

        namespace impl = converters_implementation;

        std::vector<std::optional<numerical::Vector<2, T>>> res;
        res.reserve(points.size());
        for (const std::optional<Point<N, T>>& p : points)
        {
                if (p)
                {
                        const T sd = std::sqrt(p->position_p[INDEX]);
                        if (!std::isnan(sd))
                        {
                                res.push_back({
                                        {impl::time_unit(p->time), sd}
                                });
                                continue;
                        }
                }
                res.emplace_back();
        }
        return res;
}
}
