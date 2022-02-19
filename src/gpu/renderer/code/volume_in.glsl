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

#ifndef VOLUME_IN
#define VOLUME_IN

#include "drawing_buffer.glsl"
#include "transparency.glsl"

#ifdef RAY_TRACING
#extension GL_EXT_ray_query : require
#endif

layout(set = 0, binding = 0, r32ui) uniform restrict readonly uimage2DMS transparency_heads;
layout(set = 0, binding = 1, std430) buffer restrict readonly TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

#ifdef IMAGE

DRAWING_BUFFER(0, 2);

layout(set = 0, binding = 3) uniform sampler2DMS depth_image;

layout(set = 0, binding = 4) uniform sampler2D ggx_f1_albedo_cosine_roughness;
layout(set = 0, binding = 5) uniform sampler1D ggx_f1_albedo_cosine_weighted_average;

#ifdef RAY_TRACING
layout(set = 0, binding = 6) uniform accelerationStructureEXT acceleration_structure;
#else
layout(set = 0, binding = 6) uniform sampler2D shadow_mapping_texture;
#endif

#endif

//

#ifdef IMAGE

layout(set = 1, binding = 0, std140) uniform restrict Coordinates
{
        mat4 inverse_mvp_matrix;
        mat4 texture_to_world_matrix;
        vec4 third_row_of_mvp;
        vec4 clip_plane_equation;
        vec3 gradient_h;
        mat3 gradient_to_world_matrix;
}
coordinates;

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

#ifndef RAY_TRACING
layout(set = 1, binding = 4, std140) uniform restrict ShadowMatrix
{
        mat4 texture_to_shadow;
}
shadow_matrix;
#endif

#endif

#endif
