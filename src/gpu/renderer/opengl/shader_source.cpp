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

#if defined(OPENGL_FOUND)

#include "shader_source.h"

constexpr const char triangles_vert[]{
#include "renderer_triangles.vert.str"
};
constexpr const char triangles_geom[]{
#include "renderer_triangles.geom.str"
};
constexpr const char triangles_frag[]{
#include "renderer_triangles.frag.str"
};
constexpr const char shadow_vert[]{
#include "renderer_shadow.vert.str"
};
constexpr const char shadow_frag[]{
#include "renderer_shadow.frag.str"
};
constexpr const char points_0d_vert[]{
#include "renderer_points_0d.vert.str"
};
constexpr const char points_1d_vert[]{
#include "renderer_points_1d.vert.str"
};
constexpr const char points_frag[]{
#include "renderer_points.frag.str"
};

namespace gpu_opengl
{
std::string renderer_triangles_vert()
{
        return triangles_vert;
}

std::string renderer_triangles_geom()
{
        return triangles_geom;
}

std::string renderer_triangles_frag()
{
        return triangles_frag;
}

std::string renderer_shadow_vert()
{
        return shadow_vert;
}

std::string renderer_shadow_frag()
{
        return shadow_frag;
}

std::string renderer_points_0d_vert()
{
        return points_0d_vert;
}

std::string renderer_points_1d_vert()
{
        return points_1d_vert;
}

std::string renderer_points_frag()
{
        return points_frag;
}
}

#endif
