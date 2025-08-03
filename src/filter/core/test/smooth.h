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

#include "time_update_info.h"

#include "view/point.h"

#include <vector>

namespace ns::filter::core::test
{
template <typename T>
std::vector<view::Point<T>> smooth_all(const std::vector<TimeUpdateInfo<T>>& info);

template <typename T>
std::vector<view::Point<T>> smooth_lag(const std::vector<TimeUpdateInfo<T>>& info, unsigned lag);
}
