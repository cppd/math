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
*/

const float shading_PI = 3.1415926535897932384626433832795028841971693993751;
const float shading_PI_R = 0.31830988618379067153776752674502872406891929148091;

float shading_square(const float v)
{
        return v * v;
}

// (9.16)
// Schlick approximation of Fresnel reflectance
vec3 shading_fresnel(const vec3 f0, const float h_l)
{
        return mix(f0, vec3(1), pow(1 - h_l, 5));
}

// (9.41)
// GGX distribution
float shading_ggx(const float alpha_2, const float n_h)
{
        const float v = 1 + shading_square(n_h) * (alpha_2 - 1);
        return alpha_2 / (shading_PI * shading_square(v));
}

// (9.43)
// The combined term for GGX distribution
// and the Smith masking-shadowing function
//     G2(l, v)
// -----------------
// 4 |n · l| |n · v|
float shading_ggx_g2_combined(const float alpha_2, const float n_l, const float n_v)
{
        const float lv = n_l * sqrt(alpha_2 + shading_square(n_v) * (1 - alpha_2));
        const float vl = n_v * sqrt(alpha_2 + shading_square(n_l) * (1 - alpha_2));
        return 0.5 / (lv + vl);
}

// (9.64)
// vec3 shading_diffuse(const vec3 f0, const vec3 rho_ss, const float n_l, const float n_v)
// {
//         const float l = (1 - pow(1 - n_l, 5));
//         const float v = (1 - pow(1 - n_v, 5));
//         const float c = (21 / (20 * shading_PI)) * l * v;
//         return c * (1 - f0) * rho_ss;
// }

// (9.66), (9.67) without the subsurface term
// vec3 shading_diffuse_disney_ws(
//         const vec3 /*f0*/,
//         const vec3 rho_ss,
//         const float roughness,
//         const float n_l,
//         const float n_v,
//         const float h_l)
// {
//         float l = pow(1 - n_l, 5);
//         float v = pow(1 - n_v, 5);
//         float f_d90 = 0.5 + 2 * roughness * shading_square(h_l);
//         float c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         return (c * shading_PI_R) * rho_ss;
// }
vec3 shading_diffuse_disney_ws(
        const vec3 f0,
        const vec3 rho_ss,
        const float roughness,
        const float n_l,
        const float n_v,
        const float h_l)
{
        const float l = pow(1 - n_l, 5);
        const float v = pow(1 - n_v, 5);
        const float f_d90 = 2 * roughness * shading_square(h_l);
        const float c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
        return (c * shading_PI_R) * (1 - f0) * rho_ss;
}
// (9.66), (9.67)
// vec3 shading_diffuse_disney(
//         const vec3 rho_ss,
//         const float roughness,
//         const float n_l,
//         const float n_v,
//         const float h_l,
//         const float k_ss)
// {
//         const float l = pow(1 - n_l, 5);
//         const float v = pow(1 - n_v, 5);
//         const float f_ss90 = roughness * shading_square(h_l);
//         const float f_d90 = 0.5 + 2 * f_ss90;
//         const float f_d = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         const float f_ss = (1 / (n_l * n_v) - 0.5) * (1 + (f_ss90 - 1) * l) * (1 + (f_ss90 - 1) * v) + 0.5;
//         const float c = mix(f_d, 1.25 * f_ss, k_ss);
//         return (c * shading_PI_R) * rho_ss;
// }

// (9.34)
vec3 shading_ggx_brdf(
        const float roughness,
        const vec3 f0,
        const float n_v,
        const float n_l,
        const float n_h,
        const float h_l)
{
        const float alpha = shading_square(roughness);
        const float alpha_2 = shading_square(alpha);

        return shading_fresnel(f0, h_l) * shading_ggx_g2_combined(alpha_2, n_l, n_v) * shading_ggx(alpha_2, n_h);
}

vec3 shading_ggx_diffuse(
        const float roughness,
        const vec3 f0,
        const vec3 rho_ss,
        const vec3 n,
        const vec3 v,
        const vec3 l)
{
        const float n_l = dot(n, l);
        if (n_l <= 0)
        {
                return vec3(0);
        }

        const float n_v = dot(n, v);
        if (n_v <= 0)
        {
                return vec3(0);
        }

        const vec3 h = normalize(l + v);
        const float h_l = dot(h, l);
        const float n_h = dot(n, h);

        const vec3 spec = shading_ggx_brdf(roughness, f0, n_v, n_l, n_h, h_l);
        const vec3 diff = shading_diffuse_disney_ws(f0, rho_ss, roughness, n_l, n_v, h_l);

        return n_l * (spec + diff);
}
