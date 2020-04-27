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

layout(std140, binding = 0) uniform Volume
{
        mat4 inverse_mvp_matrix;
        vec4 clip_plane_equation;
}
volume;

layout(std140, binding = 1) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        vec3 background_color;
        float normal_length;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float default_ns;
        vec3 light_a;
        vec3 light_d;
        vec3 light_s;
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
}
drawing;

layout(location = 0) out vec4 color;

bool intersect(vec3 ray_org, vec3 ray_dir, out float first, out float second)
{
        float f_max = -1e38;
        float b_min = 1e38;

        // На основе функции из ParallelotopeOrtho для случая единичного куба текстурных координат
        for (int i = 0; i < 3; ++i)
        {
                float s = ray_dir[i]; // dot(ray_dir, planes[i].n);
                if (s == 0)
                {
                        float d = ray_org[i]; // dot(ray_org, planes[i].n);
                        if (d > 1 || d < 0)
                        {
                                // параллельно плоскостям и снаружи
                                return false;
                        }
                        // внутри плоскостей
                        continue;
                }

                float d = ray_org[i]; // dot(ray_org, planes[i].n);
                float alpha1 = (1 - d) / s;
                // d и s имеют противоположный знак для другой плоскости
                float alpha2 = d / -s;

                if (s < 0)
                {
                        // пересечение снаружи для первой плоскости
                        // пересечение внутри для второй плоскости
                        f_max = max(alpha1, f_max);
                        b_min = min(alpha2, b_min);
                }
                else
                {
                        // пересечение внутри для первой плоскости
                        // пересечение снаружи для второй плоскости
                        b_min = min(alpha1, b_min);
                        f_max = max(alpha2, f_max);
                }

                if (b_min <= 0 || b_min < f_max)
                {
                        return false;
                }
        }

        // На основе функции из Parallelotope для случая одной плоскости
        if (drawing.clip_plane_enabled)
        {
                do
                {
                        vec3 n = volume.clip_plane_equation.xyz;
                        float d = volume.clip_plane_equation.w;

                        float s = dot(ray_dir, n);
                        if (s == 0)
                        {
                                if (dot(ray_org, n) > d)
                                {
                                        // параллельно плоскости и снаружи
                                        return false;
                                }
                                // внутри плоскости
                                continue;
                        }

                        float alpha = (d - dot(ray_org, n)) / s;

                        if (s < 0)
                        {
                                // пересечение снаружи
                                f_max = max(alpha, f_max);
                        }
                        else
                        {
                                // пересечение внутри
                                b_min = min(alpha, b_min);
                        }

                        if (b_min <= 0 || b_min < f_max)
                        {
                                return false;
                        }
                } while (false);
        }

        first = f_max;
        second = b_min;

        return true;
}

void main(void)
{
        vec2 device_coordinates = (gl_FragCoord.xy - drawing.viewport_center) * drawing.viewport_factor;

        vec3 ray_org = (volume.inverse_mvp_matrix * vec4(device_coordinates, 0, 1)).xyz;
        vec3 ray_dir = normalize(mat3(volume.inverse_mvp_matrix) * vec3(0, 0, 1));

        float first;
        float second;
        if (!intersect(ray_org, ray_dir, first, second))
        {
                discard;
        }

        color = vec4(0, 0.5, 0, 1);
}
