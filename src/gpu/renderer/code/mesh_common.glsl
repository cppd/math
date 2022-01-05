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

#ifndef MESH_COMMON
#define MESH_COMMON

#include "transparency.glsl"

layout(constant_id = 0) const bool TRANSPARENCY_DRAWING = false;

//

layout(set = 0, binding = 0, std140) uniform Matrices
{
        mat4 vp_matrix;
        mat4 shadow_vp_texture_matrix;
}
matrices;

layout(set = 0, binding = 1, std140) uniform Drawing
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

layout(set = 0, binding = 2) uniform sampler2D shadow_texture;
layout(set = 0, binding = 3, r32ui) uniform writeonly uimage2D object_image;

layout(set = 0, binding = 4) uniform sampler2D ggx_f1_albedo_cosine_roughness;
layout(set = 0, binding = 5) uniform sampler1D ggx_f1_albedo_cosine_weighted_average;

layout(set = 0, binding = 6, r32ui) uniform restrict uimage2DMS transparency_heads;
layout(set = 0, binding = 7, r32ui) uniform restrict uimage2DMS transparency_heads_size;
layout(set = 0, binding = 8, std430) restrict buffer TransparencyCounters
{
        uint transparency_node_counter;
        uint transparency_overload_counter;
};
layout(set = 0, binding = 9, std430) restrict writeonly buffer TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

//

layout(set = 1, binding = 0, std140) uniform Mesh
{
        mat4 model_matrix;
        mat3 normal_matrix;
        vec3 color;
        float alpha;
        float ambient;
        float metalness;
        float roughness;
}
mesh;

//

layout(set = 2, binding = 0, std140) uniform Material
{
        vec3 color;
        bool use_texture;
        bool use_material;
}
mtl;
layout(set = 2, binding = 1) uniform sampler2D material_texture;

#endif
