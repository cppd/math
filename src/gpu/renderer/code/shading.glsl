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
*/

vec3 compute_color(float metalness, float specular_power, vec3 color, vec3 N, vec3 L, vec3 V, float dot_NL)
{
        vec3 f0 = mix(vec3(0.05), color, metalness);
        vec3 diffuse_color = mix(color, vec3(0), metalness);

        float specular_k = pow(max(0, dot(V, reflect(-L, N))), specular_power);
        float m = dot(normalize(L + V), L);
        vec3 fresnel = specular_k * mix(f0, vec3(1), pow(1 - max(0, m), 5));

        return fresnel + dot_NL * (1 - fresnel) * diffuse_color;
}
