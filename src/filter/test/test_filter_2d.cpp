/*
Copyright (C) 2017-2026 Topological Manifold

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
#include "smooth.h"

#include "simulator/simulator.h"
#include "view/write.h"

#include <src/com/log.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/test/view/point.h>
#include <src/test/test.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ns::filter::test
{
namespace
{
constexpr unsigned SMOOTH_LAG = 5;

template <typename T>
constexpr T DATA_CONNECT_INTERVAL = 10;

template <std::size_t N, typename T>
view::Point<N, T> view_point(const T time, const filters::UpdateInfo<N, T>& info)
{
        return {
                .time = time,
                .position = info.position,
                .position_p = info.position_p,
                .speed = info.speed,
                .speed_p = info.speed_p,
        };
}

template <typename T>
void write_file(
        const std::string_view annotation,
        const std::vector<filters::Measurements<2, T>>& measurements,
        const Filters<T>& filters)
{
        std::vector<view::Filter<2, T>> view_filters;

        const auto push = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        view_filters.push_back(p.view_filter);
                }
        };

        const auto push_smooth = [&](const auto& v)
        {
                for (const auto& p : v)
                {
                        view_filters.push_back(p.view_filter_smooth_all);
                        view_filters.push_back(p.view_filter_smooth_lag);
                }
        };

        push(filters.positions_0);
        push(filters.positions_1);
        push(filters.positions_2);

        push_smooth(filters.positions_0);
        push_smooth(filters.positions_1);
        push_smooth(filters.positions_2);

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
                                LOG("---");
                                LOG(add_line_beginning(s, p.view_filter.name + separator));
                        }
                }
        };

        if (const auto& s = filters.position_variance->consistency_string(); !s.empty())
        {
                LOG("---");
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
void update_filters(const filters::Measurements<2, T>& m, Filters<T>* const filters)
{
        const auto& position_estimation = *filters->position_estimation;

        const auto update = [&](auto& container)
        {
                for (auto& f : container)
                {
                        const auto& info = f.filter->update(m, position_estimation);
                        if (!info)
                        {
                                continue;
                        }
                        f.view_filter.points.push_back(view_point(m.time, *info));
                }
        };

        update(filters->accelerations);
        update(filters->directions);
        update(filters->speeds);
}

template <typename T>
void update_positions(const filters::Measurements<2, T>& m, Filters<T>* const filters)
{
        const auto update = [&](auto& filters_positions)
        {
                for (auto& f : filters_positions)
                {
                        const auto& info = f.filter->update(m);
                        if (!info)
                        {
                                continue;
                        }
                        f.view_filter.points.push_back(view_point(m.time, info->info));
                        f.update_details.push_back({.time = m.time, .details = info->details});
                }
        };

        update(filters->positions_0);
        update(filters->positions_1);
        update(filters->positions_2);

        filters->position_estimation->update(m);
}

template <typename T>
void smooth_positions(Filters<T>* const filters)
{
        const auto s = [&](auto& filters_positions)
        {
                for (auto& f : filters_positions)
                {
                        f.view_filter_smooth_all.points = smooth_all(*f.filter, f.update_details);
                        f.view_filter_smooth_lag.points = smooth_lag(*f.filter, f.update_details, SMOOTH_LAG);
                }
        };

        s(filters->positions_0);
        s(filters->positions_1);
        s(filters->positions_2);
}

template <typename T>
void run(const simulator::Track<2, T>& track)
{
        Filters<T> filters = create_filters<T>();

        for (filters::Measurements<2, T> m : track.measurements())
        {
                filters.position_variance->update(m);
                if (m.position && !m.position->variance)
                {
                        m.position->variance = filters.position_variance->last_position_variance();
                }

                track.variance_correction().correct(&m);

                update_positions(m, &filters);
                update_filters(m, &filters);
        }

        smooth_positions(&filters);

        write_file(track.annotation(), track.measurements(), filters);

        write_log(filters);
}

template <typename T>
void test_impl()
{
        run(simulator::track<2, T>());
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
