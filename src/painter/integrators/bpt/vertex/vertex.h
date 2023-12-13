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

#pragma once

#include "camera.h"
#include "infinite_light.h"
#include "light.h"
#include "surface.h"

#include <cstddef>
#include <variant>

namespace ns::painter::integrators::bpt::vertex
{
template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Surface<N, T, Color>, Camera<N, T, Color>, Light<N, T, Color>, InfiniteLight<N, T, Color>>;
}
