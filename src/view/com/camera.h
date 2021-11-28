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

#pragma once

#include "../view.h"

#include <src/gpu/renderer/renderer.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>

#include <functional>
#include <mutex>

namespace ns::view
{
class Camera final
{
        mutable std::mutex lock_;

        std::function<void(const gpu::renderer::CameraInfo&)> set_camera_;

        Vector3d camera_right_{0};
        Vector3d camera_up_{0};
        Vector3d camera_direction_from_{0};

        Vector3d light_up_{0};
        Vector3d light_direction_from_{0};

        Vector2d window_center_{0};

        int width_{-1};
        int height_{-1};

        double scale_exponent_{0};
        double default_scale_{1};

        void set_vectors(const Vector3d& right, const Vector3d& up);

        gpu::renderer::CameraInfo::Volume main_volume() const;
        Matrix4d main_view_matrix() const;
        Matrix4d shadow_view_matrix() const;

        gpu::renderer::CameraInfo camera_info() const;

public:
        explicit Camera(std::function<void(const gpu::renderer::CameraInfo&)> set_camera);

        void reset(const Vector3d& right, const Vector3d& up, double scale, const Vector2d& window_center);
        void scale(double x, double y, double delta);
        void rotate(double around_up_axis, double around_right_axis);
        void move(const Vector2d& delta);
        void resize(int width, int height);

        info::Camera view_info() const;
        gpu::renderer::CameraInfo renderer_info() const;
};
}
