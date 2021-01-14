/*
Copyright (C) 2017-2021 Topological Manifold

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

/*
 Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
 Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
 Real-Time Rendering. Fourth Edition.
 CRC Press, 2018.

 9.5 Fresnel Reflectance.
 9.6 Microgeometry.
 9.7 Microfacet Theory.
 9.8 BRDF Models for Surface Reflection.
 9.9 BRDF Models for Subsurface Scattering.
*/

/*
          F(h, l) G2(l, v, h) D(h)
 f spec = ------------------------   (9.34)
             4 |n · l| |n · v|
*/

const float PI = 3.1415926535897932384626433832795028841971693993751;

float sqr(float v)
{
        return v * v;
}

// (9.16)
// Schlick approximation of Fresnel reflectance
vec3 s_fresnel(vec3 f0, float H_L)
{
        return mix(f0, vec3(1), pow(1 - H_L, 5));
}

// (9.41)
// GGX distribution
float s_d(float alpha_2, float N_H)
{
        float v = 1 + sqr(N_H) * (alpha_2 - 1);
        return N_H * alpha_2 / (PI * sqr(v));
}

// (9.43)
// The combined term for GGX distribution
// and the Smith masking-shadowing function
//     G2(l, v)
// -----------------
// 4 |n · l| |n · v|
float s_g2_combined(float alpha_2, float N_L, float N_V)
{
        float lv = N_L * sqrt(mix(sqr(N_V), 1, alpha_2));
        float vl = N_V * sqrt(mix(sqr(N_L), 1, alpha_2));
        return 0.5 / (lv + vl);
}

// (9.64)
//vec3 s_diffuse(vec3 f0, vec3 color, float N_L, float N_V)
//{
//        float l = (1 - pow(1 - N_L, 5));
//        float v = (1 - pow(1 - N_V, 5));
//        return (1 - f0) * color * ((21 / (20 * PI)) * l * v);
//}

// (9.66), (9.67) without the subsurface term
vec3 s_diffuse_disney_without_subsurface(vec3 color, float roughness, float N_L, float N_V, float H_L)
{
        float l = pow(1 - N_L, 5);
        float v = pow(1 - N_V, 5);
        float d_90 = 0.5 + 2 * roughness * sqr(H_L);
        float f_d = mix(1, d_90, l) * mix(1, d_90, v);
        return color * (N_L * N_V * (1 / PI) * f_d);
}

// (9.66), (9.67)
//vec3 s_diffuse_disney(vec3 color, float roughness, float N_L, float N_V, float H_L, float k_ss)
//{
//        float l = pow(1 - N_L, 5);
//        float v = pow(1 - N_V, 5);
//        float ss_90 = roughness * sqr(H_L);
//        float d_90 = 0.5 + 2 * ss_90;
//        float f_d = mix(1, d_90, l) * mix(1, d_90, v);
//        float f_ss = mix(1, ss_90, l) * mix(1, ss_90, v);
//        f_ss = (1/(N_L * N_V) - 0.5)*f_ss + 0.5;
//        return color * (N_L * N_V * (1 / PI) * mix(f_d, 1.25*f_ss, k_ss));
//}

vec3 shade(float intensity, float metalness, float roughness, vec3 color, vec3 N, vec3 L, vec3 V)
{
        float N_L = dot(N, L);
        if (N_L <= 0)
        {
                return vec3(0);
        }

        const float F0 = 0.05;

        const float alpha = sqr(roughness);
        const float alpha_2 = sqr(alpha);

        vec3 f0 = mix(vec3(F0), color, metalness);
        vec3 diffuse_color = mix(color, vec3(0), metalness);

        vec3 H = normalize(L + V);
        float H_L = dot(H, L);
        float N_V = dot(N, V);
        float N_H = dot(N, H);

        vec3 spec = s_fresnel(f0, H_L) * s_g2_combined(alpha_2, N_L, N_V) * s_d(alpha_2, N_H);
        vec3 diff = s_diffuse_disney_without_subsurface(diffuse_color, roughness, N_L, N_V, H_L);

        return intensity * (spec + diff);
}
