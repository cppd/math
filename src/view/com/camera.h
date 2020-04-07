/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/thread.h>
#include <src/gpu/renderer/camera_info.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>

namespace view
{
class Camera final
{
        mutable SpinLock m_lock;

        vec3 m_camera_right{0};
        vec3 m_camera_up{0};
        vec3 m_camera_direction{0}; // от камеры на объект

        vec3 m_light_up{0};
        vec3 m_light_direction{0}; // от источника света на объект

        vec2 m_window_center{0};

        int m_width = -1;
        int m_height = -1;

        double m_scale_exponent{0};
        double m_default_scale{1};

        void set_vectors(const vec3& right, const vec3& up);

        void main_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const;
        void shadow_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const;
        mat4 main_view_matrix() const;
        mat4 shadow_view_matrix() const;

public:
        void reset(const vec3& right, const vec3& up, double scale, const vec2& window_center);
        void scale(double x, double y, double delta);
        void rotate(double around_up_axis, double around_right_axis);
        void move(const vec2& delta);
        void resize(int width, int height);

        info::Camera view_info() const;
        gpu::RendererCameraInfo renderer_info() const;
};
}
