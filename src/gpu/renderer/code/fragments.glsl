/*
Copyright (C) 2017-2026 Topological Manifold

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

#ifndef FRAGMENTS_GLSL
#define FRAGMENTS_GLSL

const uint TRANSPARENCY_MAX_FRAGMENT_COUNT = 32;
const uint TRANSPARENCY_NULL_INDEX = 0xffffffff;

const vec4 OPACITY_V_1_NULL_VALUE = vec4(0);

//

struct TransparencyFragment
{
        uint color_rgba;
        uint metalness_roughness_ambient_edge_factor;
        float n_x;
        float n_y;
        float n_z;
        float depth;
#ifdef RAY_TRACING
        float world_position_x;
        float world_position_y;
        float world_position_z;
        float geometric_normal_x;
        float geometric_normal_y;
        float geometric_normal_z;
#endif
};

struct TransparencyNode
{
        TransparencyFragment fragment;
        uint next;
};

TransparencyNode create_transparency_node(
        const uint next,
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const float depth,
        const vec3 world_position,
        const vec3 geometric_normal)
{
        TransparencyNode node;

        node.fragment.color_rgba = packUnorm4x8(vec4(color, alpha));
        node.fragment.metalness_roughness_ambient_edge_factor =
                packUnorm4x8(vec4(metalness, roughness, ambient, edge_factor));

        node.fragment.n_x = n.x;
        node.fragment.n_y = n.y;
        node.fragment.n_z = n.z;
        node.fragment.depth = depth;

#ifdef RAY_TRACING
        node.fragment.world_position_x = world_position.x;
        node.fragment.world_position_y = world_position.y;
        node.fragment.world_position_z = world_position.z;
        node.fragment.geometric_normal_x = geometric_normal.x;
        node.fragment.geometric_normal_y = geometric_normal.y;
        node.fragment.geometric_normal_z = geometric_normal.z;
#endif

        node.next = next;

        return node;
}

//

struct OpacityFragment
{
        uvec2 v_0;
        vec4 v_1;
#ifdef RAY_TRACING
        vec4 v_2;
        vec2 v_3;
#endif
};

OpacityFragment create_opacity_fragment(
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const float depth,
        const vec3 world_position,
        const vec3 geometric_normal)
{
        OpacityFragment fragment;

        fragment.v_0[0] = packUnorm4x8(vec4(color.rgb, alpha));
        fragment.v_0[1] = packUnorm4x8(vec4(metalness, roughness, ambient, edge_factor));

        fragment.v_1.xyz = n;
        fragment.v_1.w = depth;

#ifdef RAY_TRACING
        fragment.v_2.xyz = world_position;
        fragment.v_2.w = geometric_normal.x;
        fragment.v_3 = geometric_normal.yz;
#endif

        return fragment;
}

bool opacity_empty(const vec4 v_1)
{
        return v_1 == OPACITY_V_1_NULL_VALUE;
}

float opacity_depth(const vec4 v_1)
{
        return v_1.w;
}

//

struct Fragment
{
        vec4 color;
        float metalness;
        float roughness;
        float ambient;
        float edge_factor;
        vec3 n;
        float depth;
#ifdef RAY_TRACING
        vec3 world_position;
        vec3 geometric_normal;
#endif
};

Fragment to_fragment(const TransparencyFragment fragment)
{
        Fragment res;

        res.color = unpackUnorm4x8(fragment.color_rgba);
        {
                const vec4 v = unpackUnorm4x8(fragment.metalness_roughness_ambient_edge_factor);
                res.metalness = v[0];
                res.roughness = v[1];
                res.ambient = v[2];
                res.edge_factor = v[3];
        }
        res.n = vec3(fragment.n_x, fragment.n_y, fragment.n_z);
        res.depth = fragment.depth;

#ifdef RAY_TRACING
        res.world_position = vec3(fragment.world_position_x, fragment.world_position_y, fragment.world_position_z);
        res.geometric_normal =
                vec3(fragment.geometric_normal_x, fragment.geometric_normal_y, fragment.geometric_normal_z);
#endif

        return res;
}

Fragment to_fragment(const OpacityFragment fragment)
{
        Fragment res;

        res.color = unpackUnorm4x8(fragment.v_0[0]);
        {
                const vec4 v = unpackUnorm4x8(fragment.v_0[1]);
                res.metalness = v[0];
                res.roughness = v[1];
                res.ambient = v[2];
                res.edge_factor = v[3];
        }
        res.n = fragment.v_1.xyz;
        res.depth = fragment.v_1.w;

#ifdef RAY_TRACING
        res.world_position = fragment.v_2.xyz;
        res.geometric_normal.x = fragment.v_2.w;
        res.geometric_normal.yz = fragment.v_3;
#endif

        return res;
}

#endif
