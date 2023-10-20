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

#include "filter/create_filters.h"
#include "filter/measurement.h"
#include "filter/simulator.h"
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

template <typename T>
void write_file(
        const std::string_view annotation,
        const std::vector<filter::Measurements<2, T>>& measurements,
        const filter::Filters<T>& filters)
{
        std::vector<view::Filter<2, T>> view_filters;

        const auto push = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        view_filters.push_back(
                                {.name = p.data.name,
                                 .color = p.data.color,
                                 .speed = p.data.speeds,
                                 .speed_p = p.data.speeds_p,
                                 .position = p.data.positions,
                                 .position_p = p.data.positions_p});
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

template <typename T>
void write_log(const filter::Filters<T>& filters)
{
        const auto log = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        const std::string s = p.filter->consistency_string(p.data.name);
                        if (!s.empty())
                        {
                                LOG(s);
                        }
                }
        };

        if (const auto& s = filters.position_variance->consistency_string("Variance"); !s.empty())
        {
                LOG(s);
        }

        log(filters.positions_0);
        log(filters.positions_1);
        log(filters.positions_2);

        log(filters.accelerations);
        log(filters.directions);
        log(filters.speeds);
}

template <typename T>
void update(const filter::Measurements<2, T>& measurement, filter::Filters<T>* const filters)
{
        const auto& position_estimation = *filters->position_estimation;

        for (auto& m : filters->accelerations)
        {
                const auto& update = m.filter->update(measurement, position_estimation);
                m.data.update(measurement.time, update);
        }

        for (auto& m : filters->directions)
        {
                const auto& update = m.filter->update(measurement, position_estimation);
                m.data.update(measurement.time, update);
        }

        for (auto& m : filters->speeds)
        {
                const auto& update = m.filter->update(measurement, position_estimation);
                m.data.update(measurement.time, update);
        }
}

template <typename T>
void update(
        filter::Measurements<2, T> measurement,
        std::vector<filter::Measurements<2, T>>* const measurements,
        filter::Filters<T>* const filters)
{
        measurements->push_back(measurement);

        filters->position_variance->update(measurement);

        if (measurement.position && !measurement.position->variance)
        {
                measurement.position->variance = filters->position_variance->last_position_variance();
        }

        for (auto& p : filters->positions_1)
        {
                const auto& update = p.filter->update(measurement);
                p.data.update(measurement.time, update);
        }

        for (auto& p : filters->positions_2)
        {
                const auto& update = p.filter->update(measurement);
                p.data.update(measurement.time, update);
        }

        filters->position_estimation->update(measurement);

        update(measurement, filters);
}

template <typename T>
void run(const filter::Track<2, T>& track)
{
        filter::Filters<T> filters = filter::create_filters<T>();
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
        run(filter::track<2, T>());
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
