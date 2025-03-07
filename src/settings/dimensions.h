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

#include <utility>

namespace ns::settings
{
using Dimensions = std::index_sequence<3, 4, 5, 6>;

using Dimensions2 = std::index_sequence<2, 3, 4, 5, 6>;

using Dimensions2A = std::index_sequence<2, 3, 4, 5, 6, 7>;
}
