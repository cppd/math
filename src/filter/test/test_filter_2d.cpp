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
        const filter::Test<T>& test)
{
        std::vector<view::Filter<2, T>> filters;

        const auto push = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        if constexpr (requires { p->name(); })
                        {
                                filters.push_back(
                                        {.name = p->name(),
                                         .color = p->color(),
                                         .speed = p->speeds(),
                                         .speed_p = p->speeds_p(),
                                         .position = p->positions(),
                                         .position_p = p->positions_p()});
                        }
                        else
                        {
                                filters.push_back(
                                        {.name = p->data.name,
                                         .color = p->data.color,
                                         .speed = p->data.speeds,
                                         .speed_p = p->data.speeds_p,
                                         .position = p->data.positions,
                                         .position_p = p->data.positions_p});
                        }
                }
        };

        push(test.positions_0);
        push(test.positions_1);
        push(test.positions_2);

        push(test.accelerations);
        push(test.directions);
        push(test.speeds);

        view::write_to_file(annotation, measurements, DATA_CONNECT_INTERVAL<T>, filters);
}

template <typename T>
void write_log(const filter::Test<T>& test)
{
        const auto log = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        const std::string s = p->filter->consistency_string(p->data.name);
                        if (!s.empty())
                        {
                                LOG(s);
                        }
                }
        };

        if (const auto& s = test.position_variance->consistency_string("Variance"); !s.empty())
        {
                LOG(s);
        }

        log(test.positions_0);
        log(test.positions_1);
        log(test.positions_2);

        log(test.accelerations);
        log(test.directions);
        log(test.speeds);
}

template <typename T>
void update(const filter::Measurements<2, T>& measurement, filter::Test<T>* const test)
{
        const auto& position_estimation = *test->position_estimation;

        for (auto& m : test->accelerations)
        {
                const auto& update = m->filter->update(measurement, position_estimation);
                m->data.update(measurement.time, update);
        }

        for (auto& m : test->directions)
        {
                const auto& update = m->filter->update(measurement, position_estimation);
                m->data.update(measurement.time, update);
        }

        for (auto& m : test->speeds)
        {
                const auto& update = m->filter->update(measurement, position_estimation);
                m->data.update(measurement.time, update);
        }
}

template <typename T>
void update(
        filter::Measurements<2, T> measurement,
        std::vector<filter::Measurements<2, T>>* const measurements,
        filter::Test<T>* const test)
{
        measurements->push_back(measurement);

        test->position_variance->update(measurement);

        if (measurement.position && !measurement.position->variance)
        {
                measurement.position->variance = test->position_variance->last_position_variance();
        }

        for (auto& p : test->positions_1)
        {
                const auto& update = p->filter->update(measurement);
                p->data.update(measurement.time, update);
        }

        for (auto& p : test->positions_2)
        {
                const auto& update = p->filter->update(measurement);
                p->data.update(measurement.time, update);
        }

        test->position_estimation->update(measurement);

        update(measurement, test);
}

template <typename T>
void run(const filter::Track<2, T>& track)
{
        filter::Test<T> test = filter::create_data<T>();
        std::vector<filter::Measurements<2, T>> measurements;

        for (const filter::Measurements<2, T>& measurement : track)
        {
                update(measurement, &measurements, &test);
        }

        write_file(track.annotation(), measurements, test);

        write_log(test);
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
