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

layout(std430, binding = 0) readonly restrict buffer StorageBufferPoints
{
        ivec2 points[];
};

layout(std430, binding = 1) readonly restrict buffer StorageBufferPointsFlow
{
        vec2 points_flow[];
};

layout(std140, binding = 2) restrict uniform Data
{
        mat4 matrix;
};

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_PointSize;
};

void main()
{
        const int vertex_index = gl_VertexIndex;

        const uint point_number = vertex_index >> 1;

        vec2 s;
        if ((vertex_index & 1) == 0)
        {
                s = points[point_number];
        }
        else
        {
                s = round(points[point_number] + points_flow[point_number]);
        }

        gl_Position = matrix * vec4(s, 0, 1);
        gl_PointSize = 1;
}
