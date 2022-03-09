/*
Copyright (C) 2017-2022 Topological Manifold

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

#ifndef TRANSPARENCY_GLSL
#define TRANSPARENCY_GLSL

const uint TRANSPARENCY_MAX_NODES = 32;
const uint TRANSPARENCY_NULL_INDEX = 0xffffffff;

struct Fragment
{
        uint color_rg;
        uint color_ba;
        float depth;
};

struct TransparencyNode
{
        Fragment fragment;
        uint next;
};

struct FragmentData
{
        vec4 color;
};

TransparencyNode create_transparency_node(const vec3 color, const float alpha, const float depth, const uint next)
{
        TransparencyNode node;

        node.fragment.color_rg = packUnorm2x16(color.rg);
        node.fragment.color_ba = packUnorm2x16(vec2(color.b, alpha));
        node.fragment.depth = depth;
        node.next = next;

        return node;
}

FragmentData fragment_data(const Fragment fragment)
{
        FragmentData data;
        data.color.rg = unpackUnorm2x16(fragment.color_rg);
        data.color.ba = unpackUnorm2x16(fragment.color_ba);
        return data;
}

#endif
