/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../detect.h"

namespace ns
{
static_assert(is_array<const std::array<int, 1>>);
static_assert(is_vector<const std::vector<int>>);

static_assert(!is_array<const std::vector<int>>);
static_assert(!is_vector<const std::array<int, 1>>);

static_assert(has_cbegin_cend<const std::array<int, 1>>);
static_assert(has_cbegin_cend<std::vector<double>&>);
static_assert(has_cbegin_cend<int[1]>);
static_assert(!has_cbegin_cend<int>);
static_assert(!has_cbegin_cend<double*>);

static_assert(has_data_and_size<const std::array<int, 1>>);
static_assert(has_data_and_size<std::vector<double>&>);
static_assert(!has_data_and_size<int>);
static_assert(!has_data_and_size<double*>);
}
