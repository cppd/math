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

#ifndef MESH_FRAGMENT_GLSL
#define MESH_FRAGMENT_GLSL

const uint TRANSPARENCY_MAX_NODES = 32;
const uint TRANSPARENCY_NULL_INDEX = 0xffffffff;

const vec4 OPACITY_V_1_NULL_VALUE = vec4(-1);

//

struct TransparencyFragment
{
        uint color_rg;
        uint color_ba;
        uint metalness_roughness_ambient_edge_factor;
        float n_x;
        float n_y;
        float n_z;
        float depth;
};

struct TransparencyNode
{
        TransparencyFragment fragment;
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
        node.fragment.metalness_roughness_ambient_edge_factor =
                packUnorm4x8(vec4(metalness, roughness, ambient, edge_factor));
        node.fragment.n_x = n.x;
        node.fragment.n_y = n.y;
        node.fragment.n_z = n.z;
        node.fragment.depth = depth;
        node.next = next;

        return node;
}

//

struct OpacityFragment
{
        uvec4 v_0;
        vec4 v_1;
};

OpacityFragment create_opacity_fragment(
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const float depth)
{
        OpacityFragment fragment;

        fragment.v_0[0] = packUnorm2x16(color.rg);
        fragment.v_0[1] = packUnorm2x16(vec2(color.b, alpha));
        fragment.v_0[2] = packUnorm2x16(vec2(metalness, roughness));
        fragment.v_0[3] = packUnorm2x16(vec2(ambient, edge_factor));

        fragment.v_1[0] = n.x;
        fragment.v_1[1] = n.y;
        fragment.v_1[2] = n.z;
        fragment.v_1[3] = depth;

        return fragment;
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

FragmentData fragment_data(const TransparencyFragment fragment)
{
        FragmentData data;

        data.color.rg = unpackUnorm2x16(fragment.color_rg);
        data.color.ba = unpackUnorm2x16(fragment.color_ba);
        {
                const vec4 v = unpackUnorm4x8(fragment.metalness_roughness_ambient_edge_factor);
                data.metalness = v[0];
                data.roughness = v[1];
                data.ambient = v[2];
                data.edge_factor = v[3];
        }
        data.n = vec3(fragment.n_x, fragment.n_y, fragment.n_z);
        data.depth = fragment.depth;

        return data;
}

float fragment_opacity_depth(const vec4 v_1)
{
        return v_1.w;
}

FragmentData fragment_data(const OpacityFragment fragment)
{
        FragmentData data;

        data.color.rg = unpackUnorm2x16(fragment.v_0[0]);
        data.color.ba = unpackUnorm2x16(fragment.v_0[1]);
        {
                const vec2 v = unpackUnorm2x16(fragment.v_0[2]);
                data.metalness = v[0];
                data.roughness = v[1];
        }
        {
                const vec2 v = unpackUnorm2x16(fragment.v_0[3]);
                data.ambient = v[0];
                data.edge_factor = v[1];
        }
        data.n = fragment.v_1.xyz;
        data.depth = fragment.v_1.w;

        return data;
}

#endif
