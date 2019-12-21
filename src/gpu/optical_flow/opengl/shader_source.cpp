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

constexpr const char sobel_comp[]{
#include "optical_flow_sobel.comp.str"
};
constexpr const char flow_comp[]{
#include "optical_flow_flow.comp.str"
};
constexpr const char downsample_comp[]{
#include "optical_flow_downsample.comp.str"
};
constexpr const char grayscale_comp[]{
#include "optical_flow_grayscale.comp.str"
};
constexpr const char show_vert[]{
#include "optical_flow_show.vert.str"
};
constexpr const char show_frag[]{
#include "optical_flow_show.frag.str"
};
constexpr const char show_debug_vert[]{
#include "optical_flow_show_debug.vert.str"
};
constexpr const char show_debug_frag[]{
#include "optical_flow_show_debug.frag.str"
};

namespace gpu_opengl
{
std::string optical_flow_sobel_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += sobel_comp;
        return s;
}

std::string optical_flow_flow_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += flow_comp;
        return s;
}

std::string optical_flow_downsample_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += downsample_comp;
        return s;
}

std::string optical_flow_grayscale_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += grayscale_comp;
        return s;
}

std::string optical_flow_show_vert()
{
        return show_vert;
}

std::string optical_flow_show_frag()
{
        return show_frag;
}

std::string optical_flow_show_debug_vert()
{
        return show_debug_vert;
}

std::string optical_flow_show_debug_frag()
{
        return show_debug_frag;
}
}

#endif
