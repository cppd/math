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
#include "measurement.h"
#include "simulator.h"

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
        const std::vector<Measurements<2, T>>& measurements,
        const Test<T>& test)
{
        std::vector<view::Filter<2, T>> filters;

        const auto push = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        filters.push_back(
                                {.name = p->name(),
                                 .color = p->color(),
                                 .speed = p->speeds(),
                                 .speed_p = p->speeds_p(),
                                 .position = p->positions(),
                                 .position_p = p->positions_p()});
                }
        };

        push(test.positions_0);
        push(test.positions_1);
        push(test.positions_2);

        push(test.processes);

        push(test.moves_1_0);
        push(test.moves_1_1);
        push(test.moves_2_1);

        view::write_to_file(annotation, measurements, DATA_CONNECT_INTERVAL<T>, filters);
}

template <typename T>
void write_log(const Test<T>& test)
{
        const auto log_consistency_string = [](const auto& p)
        {
                const std::string s = p.consistency_string();
                if (!s.empty())
                {
                        LOG(s);
                }
        };

        const auto log = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        if constexpr (requires { log_consistency_string(*p); })
                        {
                                log_consistency_string(*p);
                        }
                        else
                        {
                                log_consistency_string(p);
                        }
                }
        };

        log_consistency_string(*test.position_variance);

        log(test.positions_0);
        log(test.positions_1);
        log(test.positions_2);

        log(test.processes);

        log(test.moves_1_0);
        log(test.moves_1_1);
        log(test.moves_2_1);
}

template <typename T>
void update(Measurements<2, T> measurement, std::vector<Measurements<2, T>>* const measurements, Test<T>* const test)
{
        measurements->push_back(measurement);

        test->position_variance->update_position(measurement);

        if (measurement.position && !measurement.position->variance)
        {
                measurement.position->variance = test->position_variance->last_position_variance();
        }

        for (auto& p : test->positions_2)
        {
                p->update_position(measurement);
        }

        test->position_estimation->update(measurement);

        for (auto& p : test->positions_1)
        {
                p->update_position(measurement);
        }

        for (auto& p : test->processes)
        {
                p->update(measurement, std::as_const(*test->position_estimation));
        }

        for (auto& m : test->moves_1_0)
        {
                m->update(measurement, std::as_const(*test->position_estimation));
        }

        for (auto& m : test->moves_1_1)
        {
                m->update(measurement, std::as_const(*test->position_estimation));
        }

        for (auto& m : test->moves_2_1)
        {
                m->update(measurement, std::as_const(*test->position_estimation));
        }
}

template <typename T>
void run(const Track<2, T>& track)
{
        Test<T> test = create_data<T>();
        std::vector<Measurements<2, T>> measurements;

        for (const Measurements<2, T>& measurement : track)
        {
                update(measurement, &measurements, &test);
        }

        write_file(track.annotation(), measurements, test);

        write_log(test);
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
