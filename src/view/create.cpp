/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "create.h"

#include "vulkan/view.h"

namespace view
{
std::unique_ptr<View> create_view(
        window::WindowID parent_window,
        double parent_window_ppi,
        std::vector<Command>&& initial_commands)
{
        return create_view_impl(parent_window, parent_window_ppi, std::move(initial_commands));
}
}
