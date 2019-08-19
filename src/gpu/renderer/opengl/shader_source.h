/*
Copyright (C) 2017-2019 Topological Manifold

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

#include <string>

namespace gpu_opengl
{
std::string renderer_triangles_vert();
std::string renderer_triangles_geom();
std::string renderer_triangles_frag();
std::string renderer_shadow_vert();
std::string renderer_shadow_frag();
std::string renderer_points_0d_vert();
std::string renderer_points_1d_vert();
std::string renderer_points_frag();
}
