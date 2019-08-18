/*
Copyright (C) 2017-2019 Topological Manifold

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

layout(early_fragment_tests) in;

layout(std140, binding = 1) uniform Lighting
{
        vec3 direction_to_light;
        vec3 direction_to_camera;
        bool show_smooth;
}
lighting;
layout(std140, binding = 2) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        float default_ns;
        vec3 light_a;
        vec3 light_d;
        vec3 light_s;
        bool show_materials;
        bool show_wireframe;
        bool show_shadow;
}
drawing;

layout(bindless_sampler) uniform sampler2DShadow shadow_texture;
layout(bindless_image, r32ui) writeonly uniform uimage2D object_image;

layout(std140, binding = 3) uniform Material
{
        vec3 Ka;
        vec3 Kd;
        vec3 Ks;
        layout(bindless_sampler) sampler2D map_Ka_handle;
        layout(bindless_sampler) sampler2D map_Kd_handle;
        layout(bindless_sampler) sampler2D map_Ks_handle;
        float Ns;
        bool use_map_Ka;
        bool use_map_Kd;
        bool use_map_Ks;
        bool use_material;
}
mtl;

//

in GS
{
        vec3 normal;
        vec4 shadow_position;
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

layout(location = 0) out vec4 color;

//

bool has_texture_coordinates()
{
        // Если нет текстурных координат, то они задаются числом -1e10
        return gs.texture_coordinates[0] > -1e9;
}

float edge_factor()
{
        vec3 d = fwidth(gs.baricentric);
        vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

void main()
{
        vec3 color_a, color_d, color_s;

        if (mtl.use_material && drawing.show_materials)
        {
                if (has_texture_coordinates() && mtl.use_map_Ka)
                {
                        vec4 tex_color = texture(mtl.map_Ka_handle, gs.texture_coordinates);
                        color_a = mix(mtl.Ka, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_a = mtl.Ka;
                }

                if (has_texture_coordinates() && mtl.use_map_Kd)
                {
                        vec4 tex_color = texture(mtl.map_Kd_handle, gs.texture_coordinates);
                        color_d = mix(mtl.Kd, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_d = mtl.Kd;
                }

                if (has_texture_coordinates() && mtl.use_map_Ks)
                {
                        vec4 tex_color = texture(mtl.map_Ks_handle, gs.texture_coordinates);
                        color_s = mix(mtl.Ks, tex_color.rgb, tex_color.a);
                }
                else
                {
                        color_s = mtl.Ks;
                }
        }
        else
        {
                color_a = color_d = color_s = drawing.default_color;
        }

        vec3 N = normalize(gs.normal);
        vec3 L = lighting.direction_to_light;
        vec3 V = lighting.direction_to_camera;

        float dot_NL = dot(N, L);
        if (dot_NL >= 0.0)
        {
                color_d *= dot_NL;

                float light_reflection = max(0.0, dot(V, reflect(-L, N)));

                color_s *= pow(light_reflection, mtl.use_material && drawing.show_materials ? mtl.Ns : drawing.default_ns);
        }
        else
        {
                color_d = color_s = vec3(0);
        }

        vec3 color3;

        if (drawing.show_shadow)
        {
                float shadow = textureProj(shadow_texture, gs.shadow_position);
                color3 = color_a * drawing.light_a + shadow * (color_d * drawing.light_d + color_s * drawing.light_s);
        }
        else
        {
                color3 = color_a * drawing.light_a + color_d * drawing.light_d + color_s * drawing.light_s;
        }

        if (drawing.show_wireframe)
        {
                color3 = mix(drawing.wireframe_color, color3, edge_factor());
        }

        color = vec4(color3, 1);

        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));
}
