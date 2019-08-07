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
          m_paint_width(-1),
          m_paint_height(-1),
          m_scale_exponent(0)
{
}

void Camera::set(const vec3& right, const vec3& up, double scale)
{
        std::lock_guard lg(m_lock);

        set_vectors(right, up);

        m_scale_exponent = std::log(scale) / log(SCALE_BASE);
}

double Camera::change_scale(int delta)
{
        std::lock_guard lg(m_lock);

        if ((delta < 0 && m_scale_exponent <= SCALE_EXP_MIN) || (delta > 0 && m_scale_exponent >= SCALE_EXP_MAX) || delta == 0)
        {
                return 1;
        }

        m_scale_exponent += delta;

        return std::pow(SCALE_BASE, delta);
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

void Camera::set_view_center_and_width(const vec3& vec, double view_width, int paint_width, int paint_height)
{
        std::lock_guard lg(m_lock);

        m_view_center = vec;
        m_view_width = view_width;

        m_paint_width = paint_width;
        m_paint_height = paint_height;
}
