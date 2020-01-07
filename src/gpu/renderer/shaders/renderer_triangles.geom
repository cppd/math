/*
Copyright (C) 2017-2020 Topological Manifold

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

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140, binding = 1) uniform Lighting
{
        vec3 direction_to_light;
        vec3 direction_to_camera;
        bool show_smooth;
}
lighting;

//

layout(location = 0) in VS
{
        vec3 normal;
        vec4 shadow_position;
        vec3 orig_position;
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
        vec3 normal;
        vec4 shadow_position;
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

//

const vec3 baricentric[3] = {vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)};

vec3[3] compute_normals()
{
        vec3 geometric_normal = cross(vs[1].orig_position - vs[0].orig_position, vs[2].orig_position - vs[0].orig_position);

        vec3 normals[3];

        // Нужно направить векторы вершин грани по направлению перпендикуляра
        // к той стороне грани, которая обращена к камере.

        if (lighting.show_smooth)
        {
                // direction_to_camera - это направление на камеру.
                // normal - это вектор от камеры.
                vec3 normal = faceforward(geometric_normal, lighting.direction_to_camera, geometric_normal);

                // Повернуть векторы вершин в противоположном вектору normal направлении
                normals[0] = faceforward(vs[0].normal, normal, vs[0].normal);
                normals[1] = faceforward(vs[1].normal, normal, vs[1].normal);
                normals[2] = faceforward(vs[2].normal, normal, vs[2].normal);
        }
        else
        {
                // direction_to_camera - это направление на камеру.
                // normal - это вектор на камеру.
                vec3 normal = faceforward(geometric_normal, -lighting.direction_to_camera, geometric_normal);

                normals[0] = normal;
                normals[1] = normal;
                normals[2] = normal;
        }

        return normals;
}

void main()
{
        vec3 normals[3] = compute_normals();

        for (int i = 0; i < 3; ++i)
        {
                gl_Position = gl_in[i].gl_Position;
                gl_ClipDistance[0] = gl_in[i].gl_ClipDistance[0];

                gs.normal = normals[i];
                gs.baricentric = baricentric[i];

                gs.shadow_position = vs[i].shadow_position;
                gs.texture_coordinates = vs[i].texture_coordinates;

                EmitVertex();
        }

        EndPrimitive();
}
