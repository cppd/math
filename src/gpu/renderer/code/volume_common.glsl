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

#ifndef VOLUME_COMMON
#define VOLUME_COMMON

#include "transparency.glsl"

layout(set = 0, binding = 0, std140) uniform Drawing
{
        vec3 lighting_color;
        vec3 background_color;
        vec3 wireframe_color;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float normal_length;
        bool show_materials;
        bool show_wireframe;
        bool show_shadow;
        bool show_fog;
        bool show_smooth;
        vec3 clip_plane_color;
        vec4 clip_plane_equation;
        bool clip_plane_enabled;
        vec3 direction_to_light;
        vec3 direction_to_camera;
        vec2 viewport_center;
        vec2 viewport_factor;
        uint transparency_max_node_count;
}
drawing;

layout(set = 0, binding = 1) uniform sampler2DMS depth_image;

layout(set = 0, binding = 2) uniform sampler2D ggx_f1_albedo_cosine_roughness;
layout(set = 0, binding = 3) uniform sampler1D ggx_f1_albedo_cosine_weighted_average;

layout(set = 0, binding = 4, r32ui) uniform restrict readonly uimage2DMS transparency_heads;
layout(set = 0, binding = 5, std430) restrict readonly buffer TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

#endif
