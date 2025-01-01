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

#include <src/color/color.h>
#include <src/progress/progress_list.h>
#include <src/storage/types.h>
#include <src/view/event.h>

#include <functional>
#include <tuple>
#include <vector>

namespace ns::process
{
std::function<void(progress::RatioList*)> action_painter(
        const std::vector<storage::MeshObjectConst>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        double front_light_proportion,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color);
}
