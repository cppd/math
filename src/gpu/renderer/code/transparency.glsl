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
        uint metalness_roughness;
        uint ambient_edge_factor;
        float n_x;
        float n_y;
        float n_z;
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
        node.fragment.metalness_roughness = packUnorm2x16(vec2(metalness, roughness));
        node.fragment.ambient_edge_factor = packSnorm2x16(vec2(ambient, edge_factor));
        node.fragment.n_x = n.x;
        node.fragment.n_y = n.y;
        node.fragment.n_z = n.z;
        node.fragment.depth = depth;
        node.next = next;

        return node;
}

//

struct FragmentData
{
        vec4 color;
        float metalness;
        float roughness;
        float ambient;
        float edge_factor;
        vec3 n;
        float depth;
};

FragmentData fragment_data(const Fragment fragment)
{
        FragmentData data;

        data.color.rg = unpackUnorm2x16(fragment.color_rg);
        data.color.ba = unpackUnorm2x16(fragment.color_ba);
        {
                const vec2 v = unpackUnorm2x16(fragment.metalness_roughness);
                data.metalness = v.x;
                data.roughness = v.y;
        }
        {
                const vec2 v = unpackSnorm2x16(fragment.ambient_edge_factor);
                data.ambient = v.x;
                data.edge_factor = v.y;
        }
        data.n = vec3(fragment.n_x, fragment.n_y, fragment.n_z);
        data.depth = fragment.depth;

        return data;
}

#endif
