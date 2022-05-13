/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/constant.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/transform.h>

#include <mutex>

namespace ns::view
{
namespace
{
constexpr double SCALE_BASE = 1.1;
constexpr double SCALE_EXP_MIN = -50;
constexpr double SCALE_EXP_MAX = 100;

constexpr double PI_DIV_180 = PI<double> / 180;

constexpr gpu::renderer::CameraInfo::Volume SHADOW_VOLUME =
        {.left = -1, .right = 1, .bottom = -1, .top = 1, .near = 1, .far = -1};

constexpr double to_radians(const double angle)
{
        return angle * PI_DIV_180;
}

Vector3d rotate_vector_degree(const Vector3d& axis, const double angle_degree, const Vector3d& v)
{
        return rotate_vector(axis, to_radians(angle_degree), v);
}
}

Camera::Camera(std::function<void(const gpu::renderer::CameraInfo&)> set_camera) : set_camera_(std::move(set_camera))
{
}

void Camera::set_vectors(const Vector3d& right, const Vector3d& up)
{
        camera_up_ = up.normalized();

        camera_direction_from_ = cross(up, right).normalized();

        camera_right_ = cross(camera_direction_from_, camera_up_);

        const Vector3d light_right = rotate_vector_degree(camera_up_, -45, camera_right_);
        light_up_ = rotate_vector_degree(light_right, -45, camera_up_);

        light_direction_from_ = cross(light_up_, light_right).normalized();
}

gpu::renderer::CameraInfo::Volume Camera::main_volume() const
{
        const double scale = default_scale_ / std::pow(SCALE_BASE, scale_exponent_);

        const double left = scale * (window_center_[0] - 0.5 * width_);
        const double right = scale * (window_center_[0] + 0.5 * width_);
        const double bottom = scale * (window_center_[1] - 0.5 * height_);
        const double top = scale * (window_center_[1] + 0.5 * height_);
        const double near = 1;
        const double far = -1;

        return {.left = left, .right = right, .bottom = bottom, .top = top, .near = near, .far = far};
}

Matrix4d Camera::main_view_matrix() const
{
        return numerical::transform::look_at<double>(Vector3d(0, 0, 0), camera_direction_from_, camera_up_);
}

Matrix4d Camera::shadow_view_matrix() const
{
        return numerical::transform::look_at(Vector3d(0, 0, 0), light_direction_from_, light_up_);
}

gpu::renderer::CameraInfo Camera::camera_info() const
{
        gpu::renderer::CameraInfo v;

        v.main_volume = main_volume();
        v.shadow_volume = SHADOW_VOLUME;
        v.main_view_matrix = main_view_matrix();
        v.shadow_view_matrix = shadow_view_matrix();
        v.light_direction = light_direction_from_;
        v.camera_direction = camera_direction_from_;

        return v;
}

void Camera::reset(const Vector3d& right, const Vector3d& up, const double scale, const Vector2d& window_center)
{
        std::lock_guard lg(lock_);

        set_vectors(right, up);

        scale_exponent_ = std::log(scale) / std::log(SCALE_BASE);
        window_center_ = window_center;

        if (width_ > 0 && height_ > 0)
        {
                default_scale_ = 2.0 / std::min(width_, height_);
        }
        else
        {
                default_scale_ = 1;
        }

        set_camera_(camera_info());
}

void Camera::scale(const double x, const double y, const double delta)
{
        std::lock_guard lg(lock_);

        if (!(x < width_ && y < height_))
        {
                return;
        }
        if (!(scale_exponent_ + delta >= SCALE_EXP_MIN && scale_exponent_ + delta <= SCALE_EXP_MAX))
        {
                return;
        }
        if (delta == 0)
        {
                return;
        }

        scale_exponent_ += delta;

        const double scale_delta = std::pow(SCALE_BASE, delta);

        const Vector2d mouse_local(x - width_ * 0.5, height_ * 0.5 - y);
        const Vector2d mouse_global(mouse_local + window_center_);

        // new_center = old_center + (mouse_global * scale_delta - mouse_global)
        // center = center + mouse_global * scale_delta - mouse_global
        // center += mouse_global * (scale_delta - 1)
        window_center_ += mouse_global * (scale_delta - 1);

        set_camera_(camera_info());
}

void Camera::rotate(const double around_up_axis, const double around_right_axis)
{
        std::lock_guard lg(lock_);

        const Vector3d right = rotate_vector_degree(camera_up_, around_up_axis, camera_right_);
        const Vector3d up = rotate_vector_degree(camera_right_, around_right_axis, camera_up_);
        set_vectors(right, up);

        set_camera_(camera_info());
}

void Camera::move(const Vector2d& delta)
{
        std::lock_guard lg(lock_);

        window_center_ += delta;

        set_camera_(camera_info());
}

void Camera::resize(const int width, const int height)
{
        std::lock_guard lg(lock_);

        width_ = width;
        height_ = height;

        set_camera_(camera_info());
}

info::Camera Camera::view_info() const
{
        std::lock_guard lg(lock_);

        info::Camera v;

        v.up = camera_up_;
        v.forward = camera_direction_from_;
        v.lighting = light_direction_from_;
        v.width = width_;
        v.height = height_;

        const gpu::renderer::CameraInfo::Volume volume = main_volume();

        Vector4d volume_center;
        volume_center[0] = (volume.right + volume.left) * 0.5;
        volume_center[1] = (volume.top + volume.bottom) * 0.5;
        volume_center[2] = (volume.far + volume.near) * 0.5;
        volume_center[3] = 1.0;

        const Vector4d view_center = main_view_matrix().inverse() * volume_center;

        v.view_center = Vector3d(view_center[0], view_center[1], view_center[2]);
        v.view_width = volume.right - volume.left;

        return v;
}

gpu::renderer::CameraInfo Camera::renderer_info() const
{
        std::lock_guard lg(lock_);

        return camera_info();
}
}
