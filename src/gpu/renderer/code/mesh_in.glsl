/*
Copyright (C) 2017-2024 Topological Manifold

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

#ifndef MESH_IN_GLSL
#define MESH_IN_GLSL

#include "drawing_buffer.glsl"
#include "fragments.glsl"

#ifdef RAY_TRACING
#extension GL_EXT_ray_query : require
#endif

layout(push_constant, std430) uniform restrict PushConstants
{
        bool transparency_drawing;
};

//

DRAWING_BUFFER(0, 0)

#ifdef RAY_TRACING
layout(set = 0, binding = 2) uniform accelerationStructureEXT acceleration_structure;
#else
layout(set = 0, binding = 1, std140) uniform restrict ShadowMatrices
{
        mat4 vp_matrix;
        mat4 world_to_shadow;
}
shadow_matrices;
layout(set = 0, binding = 2) uniform sampler2D shadow_mapping_texture;
#endif

layout(set = 0, binding = 3, r32ui) uniform restrict writeonly uimage2D object_image;

layout(set = 0, binding = 4) uniform sampler2D ggx_f1_albedo_cosine_roughness;
layout(set = 0, binding = 5) uniform sampler1D ggx_f1_albedo_cosine_weighted_average;

layout(set = 0, binding = 6, r32ui) uniform restrict uimage2DMS transparency_heads;
layout(set = 0, binding = 7, r32ui) uniform restrict uimage2DMS transparency_heads_size;
layout(set = 0, binding = 8, std430) buffer restrict TransparencyCounters
{
        uint transparency_node_counter;
        uint transparency_overload_counter;
};
layout(set = 0, binding = 9, std430) buffer restrict writeonly TransparencyNodes
{
        TransparencyNode transparency_nodes[];
};

//

layout(set = 1, binding = 0, std140) uniform restrict Mesh
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

layout(set = 2, binding = 0, std140) uniform restrict Material
{
        vec3 color;
        bool use_texture;
        bool use_material;
}
mtl;

layout(set = 2, binding = 1) uniform sampler2D material_texture;

#endif
