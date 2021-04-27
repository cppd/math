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
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5 Fresnel Reflectance
9.6 Microgeometry
9.7 Microfacet Theory
9.8 BRDF Models for Surface Reflection
9.9 BRDF Models for Subsurface Scattering

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
vec3 s_fresnel(vec3 f0, float h_l)
{
        return mix(f0, vec3(1), pow(1 - h_l, 5));
}

// (9.41)
// GGX distribution
float s_ggx(float alpha_2, float n_h)
{
        float v = 1 + sqr(n_h) * (alpha_2 - 1);
        return n_h * alpha_2 / (PI * sqr(v));
}

// (9.43)
// The combined term for GGX distribution
// and the Smith masking-shadowing function
//     G2(l, v)
// -----------------
// 4 |n · l| |n · v|
float s_g2_combined(float alpha_2, float n_l, float n_v)
{
        float lv = n_l * sqrt(mix(sqr(n_v), 1, alpha_2));
        float vl = n_v * sqrt(mix(sqr(n_l), 1, alpha_2));
        return 0.5 / (lv + vl);
}

// (9.64)
//vec3 s_diffuse(vec3 f0, vec3 rho_ss, float n_l, float n_v)
//{
//        float l = (1 - pow(1 - n_l, 5));
//        float v = (1 - pow(1 - n_v, 5));
//        return (1 - f0) * rho_ss * ((21 / (20 * PI)) * l * v);
//}

// (9.66), (9.67) without the subsurface term
vec3 s_diffuse_disney_without_subsurface(vec3 rho_ss, float roughness, float n_l, float n_v, float h_l)
{
        float l = pow(1 - n_l, 5);
        float v = pow(1 - n_v, 5);
        float d_90 = 0.5 + 2 * roughness * sqr(h_l);
        float f_d = mix(1, d_90, l) * mix(1, d_90, v);
        return rho_ss * (n_l * n_v * (1 / PI) * f_d);
}

// (9.66), (9.67)
//vec3 s_diffuse_disney(vec3 rho_ss, float roughness, float n_l, float n_v, float h_l, float k_ss)
//{
//        float l = pow(1 - n_l, 5);
//        float v = pow(1 - n_v, 5);
//        float ss_90 = roughness * sqr(h_l);
//        float d_90 = 0.5 + 2 * ss_90;
//        float f_d = mix(1, d_90, l) * mix(1, d_90, v);
//        float f_ss = (1 / (n_l * n_v) - 0.5) * mix(1, ss_90, l) * mix(1, ss_90, v) + 0.5;
//        return rho_ss * (n_l * n_v * (1 / PI) * mix(f_d, 1.25 * f_ss, k_ss));
//}

vec3 shade(float metalness, float roughness, vec3 surface_color, vec3 n, vec3 v, vec3 l)
{
        float n_l = dot(n, l);
        if (n_l <= 0)
        {
                return vec3(0);
        }

        const float F0 = 0.05;

        float alpha = sqr(roughness);
        float alpha_2 = sqr(alpha);

        vec3 f0 = mix(vec3(F0), surface_color, metalness);
        vec3 rho_ss = mix(surface_color, vec3(0), metalness);

        vec3 h = normalize(l + v);

        float h_l = dot(h, l);
        float n_v = dot(n, v);
        float n_h = dot(n, h);

        vec3 spec = s_fresnel(f0, h_l) * s_g2_combined(alpha_2, n_l, n_v) * s_ggx(alpha_2, n_h);
        vec3 diff = s_diffuse_disney_without_subsurface(rho_ss, roughness, n_l, n_v, h_l);

        return n_l * (spec + diff);
}
