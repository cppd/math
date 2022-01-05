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

#version 450

#extension GL_GOOGLE_include_directive : enable
#include "mesh_common.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinates;

//

layout(location = 0) out VS
{
        vec3 world_normal;
        vec3 world_position;
        vec4 shadow_position;
        vec2 texture_coordinates;
}
vs;

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

void main()
{
        const vec4 world_coordinates = mesh.model_matrix * vec4(position, 1.0);

        const vec4 pos = matrices.vp_matrix * world_coordinates;
        gl_Position = pos;

        if (drawing.clip_plane_enabled)
        {
                const vec4 world = mesh.model_matrix * vec4(position, 1.0);
                gl_ClipDistance[0] = dot(drawing.clip_plane_equation, world);
        }
        else
        {
                gl_ClipDistance[0] = 1;
        }

        vs.world_normal = mesh.normal_matrix * normal;
        vs.world_position = position;

        vs.shadow_position = matrices.shadow_vp_texture_matrix * world_coordinates;
        vs.texture_coordinates = texture_coordinates;
}
