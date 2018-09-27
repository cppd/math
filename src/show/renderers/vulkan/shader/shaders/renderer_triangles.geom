/*
Copyright (C) 2017, 2018 Topological Manifold

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

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 1) uniform Lighting
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
        flat vec3 geometric_normal;
        vec2 texture_coordinates;
}
vs[3];

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
        vec3 normals[3];

        // Нужно направить векторы вершин грани по направлению перпендикуляра
        // к той стороне грани, которая обращена к камере.

        // Все нормали в вершинах исходных данных направлены по одну сторону
        // с перпендикуляром к грани, поэтому задавать им другое направление
        // нужно только тогда, когда нужно задать другое направление самому
        // этому перпендикуляру.

        if (lighting.show_smooth)
        {
                if (dot(lighting.direction_to_camera, vs[0].geometric_normal) >= 0)
                {
                        normals[0] = vs[0].normal;
                        normals[1] = vs[1].normal;
                        normals[2] = vs[2].normal;
                }
                else
                {
                        normals[0] = -vs[0].normal;
                        normals[1] = -vs[1].normal;
                        normals[2] = -vs[2].normal;
                }
        }
        else
        {
                vec3 normal = faceforward(vs[0].geometric_normal, -lighting.direction_to_camera, vs[0].geometric_normal);

                normals[0] = normal;
                normals[1] = normal;
                normals[2] = normal;
        }

        return normals;
}

void main(void)
{
        vec3 normals[3] = compute_normals();

        for (int i = 0; i < 3; ++i)
        {
                gl_Position = gl_in[i].gl_Position;

                gs.normal = normals[i];
                gs.baricentric = baricentric[i];

                gs.shadow_position = vs[i].shadow_position;
                gs.texture_coordinates = vs[i].texture_coordinates;

                EmitVertex();
        }

        EndPrimitive();
}
