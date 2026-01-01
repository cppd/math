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

#include <src/view/event.h>
#include <src/view/view.h>
#include <src/window/handle.h>

#include <array>
#include <memory>
#include <vector>

namespace ns::view::view
{
std::unique_ptr<View> create_view(
        window::WindowID window,
        const std::array<double, 2>& window_size_in_mm,
        std::vector<Command>&& initial_commands);
}
