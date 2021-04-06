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

#include "../interface.h"

#include <src/com/spin_lock.h>
#include <src/gpu/renderer/renderer.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>

namespace ns::view
{
class Camera final
{
        mutable SpinLock m_lock;

        vec3d m_camera_right{0};
        vec3d m_camera_up{0};
        vec3d m_camera_direction{0}; // от камеры на объект

        vec3d m_light_up{0};
        vec3d m_light_direction{0}; // от источника света на объект

        vec2d m_window_center{0};

        int m_width = -1;
        int m_height = -1;

        double m_scale_exponent{0};
        double m_default_scale{1};

        void set_vectors(const vec3d& right, const vec3d& up);

        gpu::renderer::CameraInfo::Volume main_volume() const;
        gpu::renderer::CameraInfo::Volume shadow_volume() const;
        mat4d main_view_matrix() const;
        mat4d shadow_view_matrix() const;

public:
        void reset(const vec3d& right, const vec3d& up, double scale, const vec2d& window_center);
        void scale(double x, double y, double delta);
        void rotate(double around_up_axis, double around_right_axis);
        void move(const vec2d& delta);
        void resize(int width, int height);

        info::Camera view_info() const;
        gpu::renderer::CameraInfo renderer_info() const;
};
}
