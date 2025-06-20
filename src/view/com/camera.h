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

#pragma once

#include <src/gpu/renderer/event.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <functional>

namespace ns::view::com
{
class Camera final
{
        numerical::Matrix3d light_matrix_;

        std::function<void(const gpu::renderer::CameraInfo&)> set_renderer_camera_;

        numerical::Vector3d camera_right_{0};
        numerical::Vector3d camera_up_{0};

        numerical::Matrix4d main_view_matrix_{numerical::ZERO_MATRIX};
        numerical::Matrix4d shadow_view_matrix_{numerical::ZERO_MATRIX};

        numerical::Vector2d window_center_{0};

        int width_{-1};
        int height_{-1};

        double scale_exponent_{0};
        double default_scale_{0};

        void set_rotation(const numerical::Vector3d& right, const numerical::Vector3d& up);

        void set_renderer_camera() const;

        [[nodiscard]] gpu::renderer::CameraInfo::Volume main_volume() const;

        [[nodiscard]] numerical::Vector3d camera_direction() const;
        [[nodiscard]] numerical::Vector3d light_direction() const;

public:
        explicit Camera(std::function<void(const gpu::renderer::CameraInfo&)> set_camera);

        void reset_view();
        void scale(double x, double y, double delta);
        void rotate(double around_up_axis, double around_right_axis);
        void move(const numerical::Vector2d& delta);
        void resize(int width, int height);

        [[nodiscard]] info::Camera camera() const;
        [[nodiscard]] numerical::Matrix4d view_matrix() const;
};
}
