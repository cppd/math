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

bool volume_intersect(
        const vec3 ray_org,
        const vec3 ray_dir,
        const vec3 planes_min,
        const vec3 planes_max,
        out float near,
        out float far)
{
        near = 0;
        far = 1e38;
        for (int i = 0; i < 3; ++i)
        {
                const float s = ray_dir[i];
                const float d = ray_org[i];
                if (s == 0)
                {
                        if (d < planes_min[i] || d > planes_max[i])
                        {
                                return false;
                        }
                        continue;
                }
                const bool dir_negative = (s < 0);
                const float r = 1 / s;
                const vec2 a = vec2((planes_min[i] - d) * r, (planes_max[i] - d) * r);
                near = max(near, a[int(dir_negative)]);
                far = min(far, a[int(!dir_negative)]);
                if (far < near)
                {
                        return false;
                }
        }
        return true;
}

bool clip_plane_intersect(
        const vec3 ray_org,
        const vec3 ray_dir,
        const vec4 clip_plane,
        inout float near,
        inout float far)
{
        const vec3 plane_n = clip_plane.xyz;
        const float plane_d = clip_plane.w;
        const float s = dot(ray_dir, plane_n);
        const float d = dot(ray_org, plane_n);
        if (s == 0)
        {
                if (d > plane_d)
                {
                        return false;
                }
                return true;
        }
        const float a = (plane_d - d) / s;
        if (s > 0)
        {
                far = min(a, far);
        }
        else
        {
                near = max(a, near);
        }
        if (far < near)
        {
                return false;
        }
        return true;
}
