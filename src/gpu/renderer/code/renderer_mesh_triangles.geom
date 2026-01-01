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

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "mesh_in.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

//

layout(location = 0) in VS
{
        vec3 world_normal;
        vec3 world_position;
        vec2 texture_coordinates;
}
vs[3];

in gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
}
gl_in[3];

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

layout(location = 0) out GS
{
        vec3 world_normal;
        vec3 world_position;
        flat vec3 world_geometric_normal;
        vec3 baricentric;
        vec2 texture_coordinates;
}
gs;

//

const vec3 BARICENTRIC[3] = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};

vec3 compute_geometric_normal()
{
        const vec3 e0 = vs[1].world_position - vs[0].world_position;
        const vec3 e1 = vs[2].world_position - vs[0].world_position;
        const vec3 n = cross(e0, e1);
        // to camera
        return faceforward(n, -drawing.direction_to_camera, n);
}

vec3[3] compute_normals(const vec3 n)
{
        vec3 normals[3];

        if (!drawing.flat_shading)
        {
                const vec3 n_r = -n;
                normals[0] = faceforward(vs[0].world_normal, n_r, vs[0].world_normal);
                normals[1] = faceforward(vs[1].world_normal, n_r, vs[1].world_normal);
                normals[2] = faceforward(vs[2].world_normal, n_r, vs[2].world_normal);
        }
        else
        {
                normals[0] = n;
                normals[1] = n;
                normals[2] = n;
        }

        return normals;
}

void main()
{
        const vec3 geometric_normal = normalize(compute_geometric_normal());
        const vec3 normals[3] = compute_normals(geometric_normal);

        for (int i = 0; i < 3; ++i)
        {
                gl_Position = gl_in[i].gl_Position;
                gl_ClipDistance[0] = gl_in[i].gl_ClipDistance[0];

                gs.world_normal = normals[i];
                gs.world_geometric_normal = geometric_normal;
                gs.world_position = vs[i].world_position;
                gs.world_geometric_normal = geometric_normal;
                gs.baricentric = BARICENTRIC[i];
                gs.texture_coordinates = vs[i].texture_coordinates;

                EmitVertex();
        }

        EndPrimitive();
}
