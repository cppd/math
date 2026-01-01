/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/view/event.h>

#include <functional>

namespace ns::view::com
{
class Camera final
{
        using Quaternion = numerical::QuaternionHJ<double, true>;

        const Quaternion lighting_quaternion_;
        const std::function<void(const gpu::renderer::CameraInfo&)> set_renderer_camera_;

        Quaternion quaternion_;
        numerical::Matrix3d main_rotation_matrix_;
        numerical::Matrix3d shadow_rotation_matrix_;

        numerical::Vector2d window_center_{0};

        int width_{-1};
        int height_{-1};

        double scale_exponent_{0};
        double scale_default_{0};

        void set_rotation(const Quaternion& quaternion);
        void set_renderer_camera() const;

        [[nodiscard]] gpu::renderer::CameraInfo::Volume main_volume() const;
        [[nodiscard]] numerical::Vector3d camera_up() const;
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

        struct Plane final
        {
                numerical::Vector3d normal;
                double near;
                double far;
        };

        [[nodiscard]] Plane plane() const;
};
}
