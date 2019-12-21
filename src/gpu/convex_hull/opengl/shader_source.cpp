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

#if defined(OPENGL_FOUND)

#include "shader_source.h"

constexpr const char prepare_comp[]{
#include "convex_hull_prepare.comp.str"
};
constexpr const char merge_comp[]{
#include "convex_hull_merge.comp.str"
};
constexpr const char filter_comp[]{
#include "convex_hull_filter.comp.str"
};
constexpr const char show_vert[]{
#include "convex_hull_show.vert.str"
};
constexpr const char show_frag[]{
#include "convex_hull_show.frag.str"
};

namespace gpu_opengl
{
std::string convex_hull_prepare_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += prepare_comp;
        return s;
}

std::string convex_hull_merge_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += merge_comp;
        return s;
}

std::string convex_hull_filter_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += filter_comp;
        return s;
}

std::string convex_hull_show_frag()
{
        return show_frag;
}

std::string convex_hull_show_vert()
{
        return show_vert;
}
}

#endif
