/*
Copyright (C) 2017-2025 Topological Manifold

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

#ifndef VOLUME_IN_GLSL
#define VOLUME_IN_GLSL

#include "drawing_buffer.glsl"
#include "fragments.glsl"

#ifdef RAY_TRACING
#extension GL_EXT_ray_query : require
#endif

#if defined(OPACITY)

layout(set = 0, binding = 0, rg32ui) uniform restrict readonly uimage2DMS opacity_0;
layout(set = 0, binding = 1, rgba32f) uniform restrict readonly image2DMS opacity_1;
#ifdef RAY_TRACING
layout(set = 0, binding = 2, rgba32f) uniform restrict readonly image2DMS opacity_2;
layout(set = 0, binding = 3, rg32f) uniform restrict readonly image2DMS opacity_3;
#endif

#endif

#if defined(TRANSPARENCY)

layout(set = 0, binding = 4, r32ui) uniform restrict readonly uimage2DMS transparency_heads;
layout(set = 0, binding = 5, std430) buffer restrict readonly TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

#endif

#if defined(OPACITY) || defined(TRANSPARENCY)

layout(set = 0, binding = 6, std140) uniform restrict Coordinates
{
        mat4 device_to_world;
        mat4 device_to_shadow;
        mat4 world_to_shadow;
}
coordinates;

#endif

#if defined(IMAGE)

layout(set = 0, binding = 7) uniform sampler2DMS depth_image;

#endif

DRAWING_BUFFER(0, 8)

layout(set = 0, binding = 9) uniform sampler2D ggx_f1_albedo_cosine_roughness;
layout(set = 0, binding = 10) uniform sampler1D ggx_f1_albedo_cosine_weighted_average;

#ifdef RAY_TRACING
layout(set = 0, binding = 11) uniform accelerationStructureEXT acceleration_structure;
#else
layout(set = 0, binding = 11) uniform sampler2D shadow_mapping_texture;
#endif

#if defined(IMAGE)

layout(set = 1, binding = 0, std140) uniform restrict VolumeCoordinates
{
        mat4 device_to_texture_matrix;
        mat4 texture_to_world_matrix;
        mat4 texture_to_shadow_matrix;
        vec4 third_row_of_texture_to_device;
        vec4 clip_plane;
        vec3 gradient_h;
        mat3 gradient_to_world_matrix;
        mat4 world_to_texture_matrix;
}
volume_coordinates;

layout(set = 1, binding = 1, std140) uniform restrict Volume
{
        float window_offset;
        float window_scale;
        float volume_alpha_coefficient;
        float isosurface_alpha;
        bool isosurface;
        float isovalue;
        vec3 color;
        bool color_volume;
        float ambient;
        float metalness;
        float roughness;
}
volume;

layout(set = 1, binding = 2) uniform sampler3D image;

layout(set = 1, binding = 3) uniform sampler1D transfer_function;

#endif

vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

#endif
