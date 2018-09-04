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

layout(bindless_sampler) uniform sampler2DShadow shadow_tex;

layout(bindless_image, r32i) writeonly uniform iimage2D object_img;

uniform vec4 default_color;
uniform vec4 wireframe_color;
uniform vec3 direction_to_light;
uniform vec3 direction_to_camera;
uniform float default_ns;

uniform vec4 light_a;
uniform vec4 light_d;
uniform vec4 light_s;

uniform int show_wireframe;
uniform int show_materials;
uniform int show_shadow;

in GS
{
        vec2 texture_coordinates;
        vec3 normal;
        vec4 shadow_position;
        flat int material_index;
        flat int property;
        vec3 baricentric;
}
gs;

struct Material
{
        vec3 Ka, Kd, Ks;
        layout(bindless_sampler) sampler2D map_Ka_handle, map_Kd_handle, map_Ks_handle;
        float Ns;
        // если нет текстуры, то -1
        int map_Ka, map_Kd, map_Ks;
};
layout(std430, binding = 0) buffer BufferObject
{
        Material mtl[];
};

layout(location = 0) out vec4 color;

float edge_factor()
{
        vec3 d = fwidth(gs.baricentric);
        vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

void main(void)
{
        vec4 color_a, color_d, color_s;

        if (gs.material_index >= 0 && show_materials > 0)
        {
                // материал есть

                vec4 mtl_color_a =
                        vec4(mtl[gs.material_index].Ka[0], mtl[gs.material_index].Ka[1], mtl[gs.material_index].Ka[2], 1);
                vec4 mtl_color_d =
                        vec4(mtl[gs.material_index].Kd[0], mtl[gs.material_index].Kd[1], mtl[gs.material_index].Kd[2], 1);
                vec4 mtl_color_s =
                        vec4(mtl[gs.material_index].Ks[0], mtl[gs.material_index].Ks[1], mtl[gs.material_index].Ks[2], 1);

                // gs.property & 1 > 0 - есть текстурные координаты

                if ((gs.property & 1) > 0 && mtl[gs.material_index].map_Ka >= 0)
                {
                        // если есть и текстурные координаты, и текстура
                        // vec4 tex_color = texture(textures[mtl[gs.material_index].map_Ka], gs.texture_coordinates);
                        vec4 tex_color = texture(mtl[gs.material_index].map_Ka_handle, gs.texture_coordinates);
                        color_a = mix(mtl_color_a, tex_color, tex_color.a);
                }
                else
                {
                        // если нет текстурных координат или нет текстуры, то цвет материала
                        color_a = mtl_color_a;
                }

                if ((gs.property & 1) > 0 && mtl[gs.material_index].map_Kd >= 0)
                {
                        // если есть и текстурные координаты, и текстура
                        // vec4 tex_color = texture(textures[mtl[gs.material_index].map_Kd], gs.texture_coordinates);
                        vec4 tex_color = texture(mtl[gs.material_index].map_Kd_handle, gs.texture_coordinates);
                        color_d = mix(mtl_color_d, tex_color, tex_color.a);
                }
                else
                {
                        // если нет текстурных координат или нет текстуры, то цвет материала
                        color_d = mtl_color_d;
                }

                if ((gs.property & 1) > 0 && mtl[gs.material_index].map_Ks >= 0)
                {
                        // если есть и текстурные координаты, и текстура
                        // vec4 tex_color = texture(textures[mtl[gs.material_index].map_Ks], gs.texture_coordinates);
                        vec4 tex_color = texture(mtl[gs.material_index].map_Ks_handle, gs.texture_coordinates);
                        color_s = mix(mtl_color_s, tex_color, tex_color.a);
                }
                else
                {
                        // если нет текстурных координат или нет текстуры, то цвет материала
                        color_s = mtl_color_s;
                }
        }
        else
        {
                // нет материала, цвет по умолчанию
                color_a = color_d = color_s = default_color;
        }

        vec3 N = normalize(gs.normal);
        vec3 L = direction_to_light;
        vec3 V = direction_to_camera;

        float dot_NL = dot(N, L);
        if (dot_NL >= 0.0)
        {
                color_d.rgb *= dot_NL;

                float light_reflection = max(0.0, dot(V, reflect(-L, N)));

                if (show_materials > 0 && gs.material_index >= 0)
                {
                        color_s.rgb *= pow(light_reflection, mtl[gs.material_index].Ns);
                }
                else
                {
                        color_s.rgb *= pow(light_reflection, default_ns);
                }
        }
        else
        {
                color_d.rgb = color_s.rgb = vec3(0.0);
        }

        if (show_shadow > 0)
        {
                float shadow = textureProj(shadow_tex, gs.shadow_position);
                color = color_a * light_a + (color_d * light_d + color_s * light_s) * shadow;
        }
        else
        {
                color = color_a * light_a + color_d * light_d + color_s * light_s;
        }

        if (show_wireframe > 0)
        {
                color = mix(wireframe_color, color, edge_factor());
        }

        imageStore(object_img, ivec2(gl_FragCoord.xy), ivec4(1));
}
