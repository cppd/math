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
#include "com/quaternion.h"

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

Camera::Camera()
        : m_camera_right(0),
          m_camera_up(0),
          m_camera_direction(0),
          m_light_up(0),
          m_light_direction(0),
          m_window_center(0, 0),
          m_paint_width(-1),
          m_paint_height(-1),
          m_scale_exponent(0),
          m_default_ortho_scale(1)
{
}

void Camera::reset(const vec3& right, const vec3& up, double scale, const vec2& window_center)
{
        std::lock_guard lg(m_lock);

        set_vectors(right, up);

        m_scale_exponent = std::log(scale) / log(SCALE_BASE);
        m_window_center = window_center;

        if (m_paint_width > 0 && m_paint_height > 0)
        {
                m_default_ortho_scale = 2.0 / std::min(m_paint_width, m_paint_height);
        }
        else
        {
                m_default_ortho_scale = 1;
        }
}

void Camera::scale(double x, double y, double delta)
{
        std::lock_guard lg(m_lock);

        if (!(x < m_paint_width && y < m_paint_height))
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

        vec2 mouse_local(x - m_paint_width * 0.5, m_paint_height * 0.5 - y);
        vec2 mouse_global(mouse_local + m_window_center);
        // Формула
        //   new_center = old_center + (mouse_global * scale_delta - mouse_global)
        //   -> center = center + mouse_global * scale_delta - mouse_global
        //   -> center += mouse_global * (scale_delta - 1)
        m_window_center += mouse_global * (scale_delta - 1);
}

void Camera::change_window_center(const vec2& delta)
{
        std::lock_guard lg(m_lock);

        m_window_center += delta;
}

vec2 Camera::window_center() const
{
        std::lock_guard lg(m_lock);

        return m_window_center;
}

double Camera::ortho_scale() const
{
        std::lock_guard lg(m_lock);

        return m_default_ortho_scale;
}

void Camera::get(vec3* camera_up, vec3* camera_direction, vec3* light_up, vec3* light_direction, double* scale) const
{
        std::lock_guard lg(m_lock);

        *camera_up = m_camera_up;
        *camera_direction = m_camera_direction;
        *light_up = m_light_up;
        *light_direction = m_light_direction;
        *scale = std::pow(SCALE_BASE, m_scale_exponent);
}

void Camera::camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width, int* paint_width,
                                int* paint_height) const
{
        std::lock_guard lg(m_lock);

        *camera_up = m_camera_up;
        *camera_direction = m_camera_direction;
        *view_center = m_view_center;
        *view_width = m_view_width;
        *paint_width = m_paint_width;
        *paint_height = m_paint_height;
}

vec3 Camera::light_direction() const
{
        std::lock_guard lg(m_lock);

        return m_light_direction;
}

void Camera::rotate(double around_up_axis, double around_right_axis)
{
        std::lock_guard lg(m_lock);

        vec3 right = rotate_vector_degree(m_camera_up, around_up_axis, m_camera_right);
        vec3 up = rotate_vector_degree(m_camera_right, around_right_axis, m_camera_up);
        set_vectors(right, up);
}

void Camera::set_view_center_and_width(const vec3& vec, double view_width)
{
        std::lock_guard lg(m_lock);

        m_view_center = vec;
        m_view_width = view_width;
}

void Camera::set_size(int width, int height)
{
        std::lock_guard lg(m_lock);

        m_paint_width = width;
        m_paint_height = height;
}
