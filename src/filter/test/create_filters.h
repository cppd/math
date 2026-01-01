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

#pragma once

#include "time_update_details.h"

#include "view/write.h"

#include <src/filter/filters/estimation/position_estimation.h>
#include <src/filter/filters/estimation/position_variance.h>
#include <src/filter/filters/filter.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T, std::size_t ORDER>
struct TestFilterPosition final
{
        std::unique_ptr<filters::FilterPosition<N, T, ORDER>> filter;
        view::Filter<N, T> view_filter;
        view::Filter<N, T> view_filter_smooth_all;
        view::Filter<N, T> view_filter_smooth_lag;
        std::vector<TimeUpdateDetails<N * (1 + ORDER), T>> update_details;

        TestFilterPosition(
                std::unique_ptr<filters::FilterPosition<N, T, ORDER>>&& filter,
                view::Filter<N, T> view_filter,
                view::Filter<N, T> view_filter_smooth_all,
                view::Filter<N, T> view_filter_smooth_lag)
                : filter(std::move(filter)),
                  view_filter(std::move(view_filter)),
                  view_filter_smooth_all(std::move(view_filter_smooth_all)),
                  view_filter_smooth_lag(std::move(view_filter_smooth_lag))
        {
        }
};

template <std::size_t N, typename T>
struct TestFilter final
{
        std::unique_ptr<filters::Filter<N, T>> filter;
        view::Filter<N, T> view_filter;

        TestFilter(std::unique_ptr<filters::Filter<N, T>>&& filter, view::Filter<N, T> view_filter)
                : filter(std::move(filter)),
                  view_filter(std::move(view_filter))
        {
        }
};

template <typename T>
struct Filters final
{
        std::vector<TestFilterPosition<2, T, 0>> positions_0;
        std::vector<TestFilterPosition<2, T, 1>> positions_1;
        std::vector<TestFilterPosition<2, T, 2>> positions_2;

        std::vector<TestFilter<2, T>> accelerations;
        std::vector<TestFilter<2, T>> directions;
        std::vector<TestFilter<2, T>> speeds;

        std::unique_ptr<filters::estimation::PositionVariance<2, T>> position_variance;
        std::unique_ptr<filters::estimation::PositionEstimation<2, T>> position_estimation;
};

template <typename T>
Filters<T> create_filters();
}
