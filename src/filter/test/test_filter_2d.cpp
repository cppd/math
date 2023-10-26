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

#include "create_filters.h"
#include "simulator.h"

#include "filter/measurement.h"
#include "view/write.h"

#include <src/com/log.h>
#include <src/test/test.h>

#include <optional>
#include <vector>

namespace ns::filter::test
{
namespace
{
template <typename T>
constexpr T DATA_CONNECT_INTERVAL = 10;

template <std::size_t N, typename T>
void add_info(const T time, const auto& update_info, view::Filter<N, T>* const filter_info)
{
        if (!update_info)
        {
                return;
        }

        filter_info->position.push_back({.time = time, .point = update_info->position});
        filter_info->position_p.push_back({.time = time, .point = update_info->position_p});
        filter_info->speed.push_back({.time = time, .point = Vector<1, T>(update_info->speed)});
        filter_info->speed_p.push_back({.time = time, .point = Vector<1, T>(update_info->speed_p)});
}

template <typename T>
void write_file(
        const std::string_view annotation,
        const std::vector<filter::Measurements<2, T>>& measurements,
        const Filters<T>& filters)
{
        std::vector<view::Filter<2, T>> view_filters;

        const auto push = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        view_filters.push_back(p.data);
                }
        };

        push(filters.positions_0);
        push(filters.positions_1);
        push(filters.positions_2);

        push(filters.accelerations);
        push(filters.directions);
        push(filters.speeds);

        view::write_to_file(annotation, measurements, DATA_CONNECT_INTERVAL<T>, view_filters);
}

std::string add_line_beginning(const std::string& s, const std::string& text)
{
        std::string res;
        res.reserve(s.size() + text.size());
        res += text;
        for (const char c : s)
        {
                res += c;
                if (c == '\n')
                {
                        res += text;
                }
        }
        return res;
}

template <typename T>
void write_log(const Filters<T>& filters)
{
        const std::string separator = "; ";

        const auto log = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        if (const std::string& s = p.filter->consistency_string(); !s.empty())
                        {
                                LOG(add_line_beginning(s, p.data.name + separator));
                        }
                }
        };

        if (const auto& s = filters.position_variance->consistency_string(); !s.empty())
        {
                LOG(add_line_beginning(s, "Variance" + separator));
        }

        log(filters.positions_0);
        log(filters.positions_1);
        log(filters.positions_2);

        log(filters.accelerations);
        log(filters.directions);
        log(filters.speeds);
}

template <typename T>
void update(const filter::Measurements<2, T>& measurement, Filters<T>* const filters)
{
        const auto& position_estimation = *filters->position_estimation;

        for (auto& f : filters->accelerations)
        {
                const auto& update_info = f.filter->update(measurement, position_estimation);
                add_info(measurement.time, update_info, &f.data);
        }

        for (auto& f : filters->directions)
        {
                const auto& update_info = f.filter->update(measurement, position_estimation);
                add_info(measurement.time, update_info, &f.data);
        }

        for (auto& f : filters->speeds)
        {
                const auto& update_info = f.filter->update(measurement, position_estimation);
                add_info(measurement.time, update_info, &f.data);
        }
}

template <typename T>
void update(
        filter::Measurements<2, T> measurement,
        std::vector<filter::Measurements<2, T>>* const measurements,
        Filters<T>* const filters)
{
        measurements->push_back(measurement);

        filters->position_variance->update(measurement);

        if (measurement.position && !measurement.position->variance)
        {
                measurement.position->variance = filters->position_variance->last_position_variance();
        }

        for (auto& f : filters->positions_1)
        {
                const auto& update_info = f.filter->update(measurement);
                add_info(measurement.time, update_info, &f.data);
        }

        for (auto& f : filters->positions_2)
        {
                const auto& update_info = f.filter->update(measurement);
                add_info(measurement.time, update_info, &f.data);
        }

        filters->position_estimation->update(measurement);

        update(measurement, filters);
}

template <typename T>
void run(const Track<2, T>& track)
{
        Filters<T> filters = create_filters<T>();
        std::vector<filter::Measurements<2, T>> measurements;

        for (const filter::Measurements<2, T>& measurement : track)
        {
                update(measurement, &measurements, &filters);
        }

        write_file(track.annotation(), measurements, filters);

        write_log(filters);
}

template <typename T>
void test_impl()
{
        run(track<2, T>());
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

TEST_LARGE("Filter 2D", test)
}
}
