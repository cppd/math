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

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "mesh_in.glsl"

layout(triangles) in;
layout(line_strip, max_vertices = 2) out;

//

layout(location = 0) in VS
{
        vec4 world_position;
}
vs[3];

out gl_PerVertex
{
        vec4 gl_Position;
};

//

void line(const int i0, const int i1, const int i2, const float d0, const float d1, const float d2)
{
        const vec4 from = mix(vs[i0].world_position, vs[i1].world_position, d0 / (d0 - d1));
        gl_Position = drawing.vp_matrix * from;
        EmitVertex();

        const vec4 to = mix(vs[i0].world_position, vs[i2].world_position, d0 / (d0 - d2));
        gl_Position = drawing.vp_matrix * to;
        EmitVertex();

        EndPrimitive();
}

void main()
{
        float d[3];
        float s[3];

        for (int i = 0; i < 3; ++i)
        {
                const float v = dot(drawing.clip_plane_equation, vs[i].world_position);
                d[i] = v;
                s[i] = sign(v);
        }

        if (abs(s[0] + s[1] + s[2]) >= 2)
        {
                return;
        }

        int i0;
        int i1;
        int i2;

        if (s[0] != 0 && s[0] != s[1] && s[0] != s[2])
        {
                i0 = 0;
                i1 = 1;
                i2 = 2;
        }
        else if (s[1] != 0 && s[1] != s[2] && s[1] != s[0])
        {
                i0 = 1;
                i1 = 2;
                i2 = 0;
        }
        else if (s[2] != 0 && s[2] != s[0] && s[2] != s[1])
        {
                i0 = 2;
                i1 = 0;
                i2 = 1;
        }
        else
        {
                return;
        }

        line(i0, i1, i2, d[i0], d[i1], d[i2]);
}
