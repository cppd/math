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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5 Fresnel Reflectance
*/

#ifndef SHADING_GGX_FRESNEL_GLSL
#define SHADING_GGX_FRESNEL_GLSL

// (9.16)
// Schlick approximation of Fresnel reflectance
vec3 shading_ggx_fresnel(const vec3 f0, const float h_l)
{
        return mix(f0, vec3(1), pow(1 - h_l, 5));
}

vec3 shading_ggx_fresnel_cosine_weighted_average(const vec3 f0)
{
        return f0 * (20.0 / 21.0) + vec3(1.0 / 21.0);
}

#endif
