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

#version 450

layout(triangles) in;
layout(line_strip, max_vertices = 2) out;

//

layout(std140, binding = 0) uniform Matrices
{
        mat4 main_mvp_matrix;
        mat4 shadow_mvp_matrix;
        mat4 shadow_mvp_texture_matrix;
        vec4 clip_plane_equation;
        vec4 clip_plane_equation_shadow;
        bool clip_plane_enabled;
}
matrices;

in gl_PerVertex
{
        vec4 gl_Position;
}
gl_in[3];

out gl_PerVertex
{
        vec4 gl_Position;
};

//

void line(int i0, int i1, int i2, float d0, float d1, float d2)
{
        gl_Position = mix(gl_in[i0].gl_Position, gl_in[i1].gl_Position, d0 / (d0 - d1));
        EmitVertex();

        gl_Position = mix(gl_in[i0].gl_Position, gl_in[i2].gl_Position, d0 / (d0 - d2));
        EmitVertex();

        EndPrimitive();
}

void main()
{
        float d[3];
        float s[3];

        for (int i = 0; i < 3; ++i)
        {
                float v = dot(matrices.clip_plane_equation, gl_in[i].gl_Position);
                d[i] = v;
                s[i] = sign(v);
        }

        if (abs(s[0] + s[1] + s[2]) >= 2)
        {
                return;
        }

        int i0, i1, i2;

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
