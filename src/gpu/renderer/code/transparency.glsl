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

//

struct Fragment
{
        uint color_rg;
        uint color_ba;
        uint n_xy;
        uint n_z_metalness;
        uint roughness_ambient;
        float edge_factor;
        float depth;
};

struct TransparencyNode
{
        Fragment fragment;
        uint next;
};

TransparencyNode create_transparency_node(
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const float depth,
        const uint next)
{
        TransparencyNode node;

        node.fragment.color_rg = packUnorm2x16(color.rg);
        node.fragment.color_ba = packUnorm2x16(vec2(color.b, alpha));
        node.fragment.n_xy = packUnorm2x16(n.xy);
        node.fragment.n_z_metalness = packUnorm2x16(vec2(n.z, metalness));
        node.fragment.roughness_ambient = packUnorm2x16(vec2(roughness, ambient));
        node.fragment.edge_factor = edge_factor;
        node.fragment.depth = depth;
        node.next = next;

        return node;
}

//

struct FragmentData
{
        vec4 color;
        vec3 n;
        float metalness;
        float roughness;
        float ambient;
        float edge_factor;
        float depth;
};

FragmentData fragment_data(const Fragment fragment)
{
        FragmentData data;

        data.color.rg = unpackUnorm2x16(fragment.color_rg);
        data.color.ba = unpackUnorm2x16(fragment.color_ba);
        data.n.xy = unpackUnorm2x16(fragment.n_xy);
        {
                const vec2 v = unpackUnorm2x16(fragment.n_z_metalness);
                data.n.z = v.x;
                data.metalness = v.y;
        }
        {
                const vec2 v = unpackUnorm2x16(fragment.roughness_ambient);
                data.roughness = v.x;
                data.ambient = v.y;
        }
        data.edge_factor = fragment.edge_factor;
        data.depth = fragment.depth;

        return data;
}

#endif
