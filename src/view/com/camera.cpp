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

namespace ns::view::com
{
namespace
{
constexpr double SCALE_BASE = 1.1;
constexpr double SCALE_EXP_MIN = -50;
constexpr double SCALE_EXP_MAX = 100;

constexpr gpu::renderer::CameraInfo::Volume SHADOW_VOLUME = {
        .left = -1,
        .right = 1,
        .bottom = -1,
        .top = 1,
        .near = 1,
        .far = -1,
};

numerical::Matrix4d rotation_to_view(const numerical::Matrix3d& m)
{
        return {
                {m[0, 0], m[0, 1], m[0, 2], 0},
                {m[1, 0], m[1, 1], m[1, 2], 0},
                {m[2, 0], m[2, 1], m[2, 2], 0},
                {0, 0, 0, 1},
        };
}

numerical::Vector3d rotate_vector_degree(
        const numerical::Vector3d& axis,
        const double angle_degrees,
        const numerical::Vector3d& v)
{
        return numerical::transform::rotate(degrees_to_radians(angle_degrees), axis, v);
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
        : set_renderer_camera_(std::move(set_camera))
{
        const auto rotation = [](const double angle_degrees, const numerical::Vector3d& axis)
        {
                return numerical::QuaternionHJ<double, true>::rotation_quaternion(
                        degrees_to_radians(angle_degrees), axis);
        };

        light_matrix_ = (rotation(-45.0, {1, 0, 0}) * rotation(-45.0, {0, 1, 0})).rotation_matrix();

        reset_view();
}

numerical::Vector3d Camera::camera_direction() const
{
        return -main_view_matrix_.row(2).head<3>();
}

numerical::Vector3d Camera::light_direction() const
{
        return -shadow_view_matrix_.row(2).head<3>();
}

void Camera::set_rotation(const numerical::Vector3d& right, const numerical::Vector3d& up)
{
        const numerical::Vector3d camera_direction = cross(up, right).normalized();
        camera_up_ = up.normalized();
        camera_right_ = cross(camera_direction, camera_up_);

        const numerical::Matrix3d main_matrix({camera_right_, camera_up_, -camera_direction});
        const numerical::Matrix3d shadow_matrix = light_matrix_ * main_matrix;

        main_view_matrix_ = rotation_to_view(main_matrix);
        shadow_view_matrix_ = rotation_to_view(shadow_matrix);
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

        return {
                .left = left,
                .right = right,
                .bottom = bottom,
                .top = top,
                .near = near,
                .far = far,
        };
}

void Camera::set_renderer_camera() const
{
        set_renderer_camera_({
                .main_volume = main_volume(),
                .shadow_volume = SHADOW_VOLUME,
                .main_view_matrix = main_view_matrix_,
                .shadow_view_matrix = shadow_view_matrix_,
                .light_direction = light_direction(),
                .camera_direction = camera_direction(),
        });
}

void Camera::reset_view()
{
        constexpr numerical::Vector3d RIGHT{1, 0, 0};
        constexpr numerical::Vector3d UP{0, 1, 0};
        constexpr double SCALE{1};
        constexpr numerical::Vector2d WINDOW_CENTER{0, 0};

        set_rotation(RIGHT, UP);

        scale_exponent_ = std::log(SCALE) / std::log(SCALE_BASE);
        window_center_ = WINDOW_CENTER;
        default_scale_ = default_scale(width_, height_);

        set_renderer_camera();
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

        const numerical::Vector2d mouse_local(x - width_ * 0.5, height_ * 0.5 - y);
        const numerical::Vector2d mouse_global(mouse_local + window_center_);

        // new_center = old_center + (mouse_global * scale_delta - mouse_global)
        // center = center + mouse_global * scale_delta - mouse_global
        // center += mouse_global * (scale_delta - 1)
        window_center_ += mouse_global * (scale_delta - 1);

        set_renderer_camera();
}

void Camera::rotate(const double around_up_axis, const double around_right_axis)
{
        const numerical::Vector3d right = rotate_vector_degree(camera_up_, around_up_axis, camera_right_);
        const numerical::Vector3d up = rotate_vector_degree(right, around_right_axis, camera_up_);

        set_rotation(right, up);

        set_renderer_camera();
}

void Camera::move(const numerical::Vector2d& delta)
{
        window_center_ += delta;

        set_renderer_camera();
}

void Camera::resize(const int width, const int height)
{
        width_ = width;
        height_ = height;

        set_renderer_camera();
}

info::Camera Camera::camera() const
{
        const gpu::renderer::CameraInfo::Volume volume = main_volume();

        const numerical::Vector4d volume_center = {
                (volume.right + volume.left) / 2,
                (volume.top + volume.bottom) / 2,
                (volume.far + volume.near) / 2,
                1,
        };

        const numerical::Vector4d view_center = main_view_matrix_.inversed() * volume_center;

        return {
                .up = camera_up_,
                .forward = camera_direction(),
                .lighting = light_direction(),
                .view_center = numerical::Vector3d(view_center[0], view_center[1], view_center[2]),
                .view_width = volume.right - volume.left,
                .width = width_,
                .height = height_,
        };
}

numerical::Matrix4d Camera::view_matrix() const
{
        return main_view_matrix_;
}
}
