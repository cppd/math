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

9.5.2 Typical Fresnel Reflectance Values
Parameterizing Fresnel Values
*/

#ifndef SHADING_GGX_METALNESS_GLSL
#define SHADING_GGX_METALNESS_GLSL

vec3 shading_ggx_compute_metalness_f0(const vec3 surface_color, const float metalness)
{
        const vec3 f0 = vec3(0.05);
        return mix(f0, surface_color, metalness);
}

vec3 shading_ggx_compute_metalness_rho_ss(const vec3 surface_color, const float metalness)
{
        return mix(surface_color, vec3(0), metalness);
}

#endif
