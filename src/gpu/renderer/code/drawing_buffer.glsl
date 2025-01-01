/*
Copyright (C) 2017-2025 Topological Manifold

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

#ifndef DRAWING_BUFFER_GLSL
#define DRAWING_BUFFER_GLSL

#define DRAWING_BUFFER(set_number, binding_number)                                          \
        layout(set = set_number, binding = binding_number, std140) uniform restrict Drawing \
        {                                                                                   \
                mat4 vp_matrix;                                                             \
                vec3 lighting_color;                                                        \
                vec3 background_color;                                                      \
                vec3 wireframe_color;                                                       \
                bool show_wireframe;                                                        \
                vec3 normal_color_positive;                                                 \
                float normal_length;                                                        \
                vec3 normal_color_negative;                                                 \
                bool show_materials;                                                        \
                bool show_shadow;                                                           \
                bool show_fog;                                                              \
                bool flat_shading;                                                          \
                uint transparency_max_node_count;                                           \
                vec3 clip_plane_color;                                                      \
                bool clip_plane_enabled;                                                    \
                vec4 clip_plane_equation;                                                   \
                vec3 direction_to_light;                                                    \
                vec3 direction_to_camera;                                                   \
                vec2 viewport_center;                                                       \
                vec2 viewport_factor;                                                       \
                float front_lighting_proportion;                                            \
                float side_lighting_proportion;                                             \
        }                                                                                   \
        drawing;

#endif
