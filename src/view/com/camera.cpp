/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/conversion.h>
#include <src/gpu/renderer/event.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace ns::view
{
namespace
{
constexpr double SCALE_BASE = 1.1;
constexpr double SCALE_EXP_MIN = -50;
constexpr double SCALE_EXP_MAX = 100;

constexpr gpu::renderer::CameraInfo::Volume SHADOW_VOLUME =
        {.left = -1, .right = 1, .bottom = -1, .top = 1, .near = 1, .far = -1};

Vector3d rotate_vector_degree(const Vector3d& axis, const double angle_degrees, const Vector3d& v)
{
        return numerical::rotate_vector(axis, degrees_to_radians(angle_degrees), v);
}

double default_scale(const int width, const int height)
{
        if (width > 0 && height > 0)
        {
                return 2.0 / std::min(width, height);
        }
        return 1;
}
}

Camera::Camera(std::function<void(const gpu::renderer::CameraInfo&)> set_camera)
        : set_camera_(std::move(set_camera))
{
        reset_view();
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

gpu::renderer::CameraInfo Camera::renderer_camera_info() const
{
        return {.main_volume = main_volume(),
                .shadow_volume = SHADOW_VOLUME,
                .main_view_matrix = main_view_matrix(),
                .shadow_view_matrix = shadow_view_matrix(),
                .light_direction = light_direction_from_,
                .camera_direction = camera_direction_from_};
}

void Camera::reset_view()
{
        constexpr Vector3d RIGHT{1, 0, 0};
        constexpr Vector3d UP{0, 1, 0};
        constexpr double SCALE{1};
        constexpr Vector2d WINDOW_CENTER{0, 0};

        set_vectors(RIGHT, UP);

        scale_exponent_ = std::log(SCALE) / std::log(SCALE_BASE);
        window_center_ = WINDOW_CENTER;
        default_scale_ = default_scale(width_, height_);

        set_camera_(renderer_camera_info());
}

void Camera::scale(const double x, const double y, const double delta)
{
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

        set_camera_(renderer_camera_info());
}

void Camera::rotate(const double around_up_axis, const double around_right_axis)
{
        const Vector3d right = rotate_vector_degree(camera_up_, around_up_axis, camera_right_);
        const Vector3d up = rotate_vector_degree(camera_right_, around_right_axis, camera_up_);
        set_vectors(right, up);

        set_camera_(renderer_camera_info());
}

void Camera::move(const Vector2d& delta)
{
        window_center_ += delta;

        set_camera_(renderer_camera_info());
}

void Camera::resize(const int width, const int height)
{
        width_ = width;
        height_ = height;

        set_camera_(renderer_camera_info());
}

info::Camera Camera::camera() const
{
        const gpu::renderer::CameraInfo::Volume volume = main_volume();

        const Vector4d volume_center = [&]
        {
                Vector4d res;
                res[0] = (volume.right + volume.left) * 0.5;
                res[1] = (volume.top + volume.bottom) * 0.5;
                res[2] = (volume.far + volume.near) * 0.5;
                res[3] = 1;
                return res;
        }();

        const Vector4d view_center = main_view_matrix().inversed() * volume_center;

        return {
                .up = camera_up_,
                .forward = camera_direction_from_,
                .lighting = light_direction_from_,
                .view_center = Vector3d(view_center[0], view_center[1], view_center[2]),
                .view_width = volume.right - volume.left,
                .width = width_,
                .height = height_,
        };
}

Matrix4d Camera::view_matrix() const
{
        return main_view_matrix();
}
}
