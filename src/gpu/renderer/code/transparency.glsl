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

struct TransparencyNode
{
        uint color_rg;
        uint color_ba;
        float depth;
        uint next;
};

struct Fragment
{
        uint color_rg;
        uint color_ba;
        float depth;
};

TransparencyNode create_transparency_node(const vec3 color, const float alpha, const float depth, const uint next)
{
        TransparencyNode node;

        node.color_rg = packUnorm2x16(color.rg);
        node.color_ba = packUnorm2x16(vec2(color.b, alpha));
        node.depth = depth;
        node.next = next;

        return node;
}

Fragment transparency_node_to_fragment(const TransparencyNode node)
{
        Fragment fragment;

        fragment.color_rg = node.color_rg;
        fragment.color_ba = node.color_ba;
        fragment.depth = node.depth;

        return fragment;
}

vec4 fragment_color(const Fragment fragment)
{
        const vec2 rg = unpackUnorm2x16(fragment.color_rg);
        const vec2 ba = unpackUnorm2x16(fragment.color_ba);
        return vec4(rg, ba);
}

#endif
