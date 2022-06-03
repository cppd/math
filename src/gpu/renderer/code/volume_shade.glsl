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

#ifndef VOLUME_SHADE_GLSL
#define VOLUME_SHADE_GLSL

#include "fragments.glsl"
#include "shade.glsl"
#include "volume_image.glsl"
#include "volume_in.glsl"
#include "volume_shadow.glsl"

#if defined(IMAGE)

vec4 isosurface_color(const vec3 texture_position)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        const vec3 world_normal =
                normalize(volume_coordinates.gradient_to_world_matrix * volume_gradient(texture_position));
        const vec3 n = faceforward(world_normal, -v, world_normal);

        vec3 color = volume.ambient * volume.color;

        if (drawing.show_shadow)
        {
                const float shadow_transparency =
                        dot(n, l) > 0 ? shadow_transparency_texture(texture_position, /*mesh_self_intersection*/ false)
                                      : 0;

                color +=
                        shade(volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, shadow_transparency);
        }
        else
        {
                color +=
                        shade(volume.color, volume.metalness, volume.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color);
        }

        return vec4(color, volume.isosurface_alpha);
}

#endif

#if defined(OPACITY) || defined(TRANSPARENCY)

vec4 fragment_color(const Fragment fragment)
{
        if (fragment.n == vec3(0))
        {
                return fragment.color;
        }

        const vec3 n = fragment.n;
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        vec3 color = fragment.color.rgb * fragment.ambient;

        if (drawing.show_shadow)
        {
                const float shadow_transparency =
                        dot(n, l) > 0 ? shadow_transparency_device(
                                vec3(device_coordinates, fragment.depth),
                                /*mesh_self_intersection*/ !fragment.normal_directed_to_light)
                                      : 0;

                color +=
                        shade(fragment.color.rgb, fragment.metalness, fragment.roughness, n, v, l,
                              ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average,
                              drawing.lighting_color, shadow_transparency);
        }
        else
        {
                color += shade(
                        fragment.color.rgb, fragment.metalness, fragment.roughness, n, v, l,
                        ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color);
        }

        if (fragment.edge_factor > 0)
        {
                color = mix(color, drawing.wireframe_color, fragment.edge_factor);
        }

        return vec4(color, fragment.color.a);
}

#endif

#if defined(TRANSPARENCY)
vec4 fragment_color(const TransparencyFragment fragment)
{
        return fragment_color(to_fragment(fragment));
}
#else
vec4 fragment_color(const TransparencyFragment fragment)
{
        return vec4(0);
}
#endif

#if defined(OPACITY)
vec4 fragment_color(const OpacityFragment fragment)
{
        return fragment_color(to_fragment(fragment));
}
#else
vec4 fragment_color(const OpacityFragment fragment)
{
        return vec4(0);
}
#endif

#endif
