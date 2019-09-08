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

#include "camera.h"

#include "com/math.h"
#include "com/matrix_alg.h"
#include "com/quaternion.h"
#include "numerical/linear.h"

#include <mutex>

constexpr double SCALE_BASE = 1.1;
constexpr double SCALE_EXP_MIN = -50;
constexpr double SCALE_EXP_MAX = 100;

constexpr double PI_DIV_180 = PI<double> / 180;

constexpr double to_radians(double angle)
{
        return angle * PI_DIV_180;
}

namespace
{
vec3 rotate_vector_degree(const vec3& axis, double angle_degree, const vec3& v)
{
        return rotate_vector(axis, to_radians(angle_degree), v);
}
}

void Camera::set_vectors(const vec3& right, const vec3& up)
{
        m_camera_up = normalize(up);

        m_camera_direction = cross(m_camera_up, normalize(right));

        m_camera_right = cross(m_camera_direction, m_camera_up);

        vec3 light_right = rotate_vector_degree(m_camera_up, -45, m_camera_right);
        m_light_up = rotate_vector_degree(light_right, -45, m_camera_up);

        m_light_direction = cross(m_light_up, light_right);
}

void Camera::view_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const
{
        double scale = m_default_scale / std::pow(SCALE_BASE, m_scale_exponent);

        *left = scale * (m_window_center[0] - 0.5 * m_width);
        *right = scale * (m_window_center[0] + 0.5 * m_width);
        *bottom = scale * (m_window_center[1] - 0.5 * m_height);
        *top = scale * (m_window_center[1] + 0.5 * m_height);
        *near = 1;
        *far = -1;
}

void Camera::shadow_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const
{
        *left = -1;
        *right = 1;
        *bottom = -1;
        *top = 1;
        *near = 1;
        *far = -1;
}

mat4 Camera::view_matrix() const
{
        return look_at<double>(vec3(0, 0, 0), m_camera_direction, m_camera_up);
}

mat4 Camera::shadow_matrix() const
{
        return look_at(vec3(0, 0, 0), m_light_direction, m_light_up);
}

void Camera::reset(const vec3& right, const vec3& up, double scale, const vec2& window_center)
{
        std::lock_guard lg(m_lock);

        set_vectors(right, up);

        m_scale_exponent = std::log(scale) / log(SCALE_BASE);
        m_window_center = window_center;

        if (m_width > 0 && m_height > 0)
        {
                m_default_scale = 2.0 / std::min(m_width, m_height);
        }
        else
        {
                m_default_scale = 1;
        }
}

void Camera::scale(double x, double y, double delta)
{
        std::lock_guard lg(m_lock);

        if (!(x < m_width && y < m_height))
        {
                return;
        }
        if (!(m_scale_exponent + delta >= SCALE_EXP_MIN && m_scale_exponent + delta <= SCALE_EXP_MAX))
        {
                return;
        }
        if (delta == 0)
        {
                return;
        }

        m_scale_exponent += delta;
        double scale_delta = std::pow(SCALE_BASE, delta);

        vec2 mouse_local(x - m_width * 0.5, m_height * 0.5 - y);
        vec2 mouse_global(mouse_local + m_window_center);
        // Формула
        //   new_center = old_center + (mouse_global * scale_delta - mouse_global)
        //   -> center = center + mouse_global * scale_delta - mouse_global
        //   -> center += mouse_global * (scale_delta - 1)
        m_window_center += mouse_global * (scale_delta - 1);
}

void Camera::rotate(double around_up_axis, double around_right_axis)
{
        std::lock_guard lg(m_lock);

        vec3 right = rotate_vector_degree(m_camera_up, around_up_axis, m_camera_right);
        vec3 up = rotate_vector_degree(m_camera_right, around_right_axis, m_camera_up);
        set_vectors(right, up);
}

void Camera::move(const vec2& delta)
{
        std::lock_guard lg(m_lock);

        m_window_center += delta;
}

void Camera::resize(int width, int height)
{
        std::lock_guard lg(m_lock);

        m_width = width;
        m_height = height;
}

ShowCameraInfo Camera::show_info() const
{
        std::lock_guard lg(m_lock);

        ShowCameraInfo v;

        v.camera_up = m_camera_up;
        v.camera_direction = m_camera_direction;
        v.light_direction = m_light_direction;
        v.width = m_width;
        v.height = m_height;

        double left, right, bottom, top, near, far;
        view_volume(&left, &right, &bottom, &top, &near, &far);
        vec4 volume_center_4((right + left) * 0.5, (top + bottom) * 0.5, (far + near) * 0.5, 1.0);
        vec4 view_center_4 = numerical::inverse(view_matrix()) * volume_center_4;

        v.view_center = vec3(view_center_4[0], view_center_4[1], view_center_4[2]);
        v.view_width = right - left;

        return v;
}

RendererCameraInfo Camera::renderer_info() const
{
        std::lock_guard lg(m_lock);

        RendererCameraInfo v;

        view_volume(&v.view_volume.left, &v.view_volume.right, &v.view_volume.bottom, &v.view_volume.top, &v.view_volume.near,
                    &v.view_volume.far);
        shadow_volume(&v.shadow_volume.left, &v.shadow_volume.right, &v.shadow_volume.bottom, &v.shadow_volume.top,
                      &v.shadow_volume.near, &v.shadow_volume.far);
        v.view_matrix = view_matrix();
        v.shadow_matrix = shadow_matrix();
        v.light_direction = m_light_direction;
        v.camera_direction = m_camera_direction;

        return v;
}
