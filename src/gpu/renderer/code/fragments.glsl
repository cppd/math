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
#ifdef RAY_TRACING
        uint geometric_normal_directed_to_light;
        float ray_org_to_light_x;
        float ray_org_to_light_y;
        float ray_org_to_light_z;
#endif
        float depth;
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
        const bool geometric_normal_directed_to_light,
        const vec3 ray_org_to_light)
{
        TransparencyNode node;

        node.fragment.color_rgba = packUnorm4x8(vec4(color, alpha));
        node.fragment.metalness_roughness_ambient_edge_factor =
                packUnorm4x8(vec4(metalness, roughness, ambient, edge_factor));
        node.fragment.n_x = n.x;
        node.fragment.n_y = n.y;
        node.fragment.n_z = n.z;

#ifdef RAY_TRACING
        node.fragment.geometric_normal_directed_to_light = geometric_normal_directed_to_light ? 1 : 0;
        node.fragment.ray_org_to_light_x = ray_org_to_light.x;
        node.fragment.ray_org_to_light_y = ray_org_to_light.y;
        node.fragment.ray_org_to_light_z = ray_org_to_light.z;
#endif

        node.fragment.depth = depth;

        node.next = next;

        return node;
}

//

struct OpacityFragment
{
        uvec4 v_0;
        vec4 v_1;
#ifdef RAY_TRACING
        vec4 v_2;
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
        const bool geometric_normal_directed_to_light,
        const vec3 ray_org_to_light)
{
        OpacityFragment fragment;

        fragment.v_0[0] = packUnorm2x16(color.rg);
        fragment.v_0[1] = packUnorm2x16(vec2(color.b, alpha));
        fragment.v_0[2] = packUnorm4x8(vec4(metalness, roughness, ambient, edge_factor));

        fragment.v_1.xyz = n;
        fragment.v_1.w = depth;

#ifdef RAY_TRACING
        fragment.v_0[3] = geometric_normal_directed_to_light ? 1 : 0;
        fragment.v_2.xyz = ray_org_to_light;
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
        bool geometric_normal_directed_to_light;
        vec3 ray_org_to_light;
};

Fragment to_fragment(const TransparencyFragment fragment)
{
        Fragment data;

        data.color = unpackUnorm4x8(fragment.color_rgba);
        {
                const vec4 v = unpackUnorm4x8(fragment.metalness_roughness_ambient_edge_factor);
                data.metalness = v[0];
                data.roughness = v[1];
                data.ambient = v[2];
                data.edge_factor = v[3];
        }
        data.n = vec3(fragment.n_x, fragment.n_y, fragment.n_z);
        data.depth = fragment.depth;

#ifdef RAY_TRACING
        data.geometric_normal_directed_to_light = fragment.geometric_normal_directed_to_light != 0;
        data.ray_org_to_light =
                vec3(fragment.ray_org_to_light_x, fragment.ray_org_to_light_y, fragment.ray_org_to_light_z);
#endif

        return data;
}

Fragment to_fragment(const OpacityFragment fragment)
{
        Fragment data;

        data.color.rg = unpackUnorm2x16(fragment.v_0[0]);
        data.color.ba = unpackUnorm2x16(fragment.v_0[1]);
        {
                const vec4 v = unpackUnorm4x8(fragment.v_0[2]);
                data.metalness = v[0];
                data.roughness = v[1];
                data.ambient = v[2];
                data.edge_factor = v[3];
        }
        data.n = fragment.v_1.xyz;
        data.depth = fragment.v_1.w;

#ifdef RAY_TRACING
        data.geometric_normal_directed_to_light = fragment.v_0[3] != 0;
        data.ray_org_to_light = fragment.v_2.xyz;
#endif

        return data;
}

#endif
