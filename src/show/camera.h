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

#pragma once

#include "com/matrix.h"
#include "com/thread.h"
#include "com/vec.h"

class Camera final
{
        mutable SpinLock m_lock;

        vec3 m_camera_right;
        vec3 m_camera_up;
        vec3 m_camera_direction; // от камеры на объект

        vec3 m_light_up;
        vec3 m_light_direction; // от источника света на объект

        vec2 m_window_center;

        int m_width;
        int m_height;

        double m_scale_exponent;
        double m_default_scale;

        void set_vectors(const vec3& right, const vec3& up);

        void view_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const;
        void shadow_volume(double* left, double* right, double* bottom, double* top, double* near, double* far) const;
        mat4 view_matrix() const;
        mat4 shadow_matrix() const;

public:
        Camera();

        struct Information
        {
                struct Volume
                {
                        double left, right, bottom, top, near, far;
                };

                Volume view_volume;
                Volume shadow_volume;
                mat4 view_matrix;
                mat4 shadow_matrix;
                vec3 light_direction;
                vec3 camera_direction;
        };

        void reset(const vec3& right, const vec3& up, double scale, const vec2& window_center);
        void scale(double x, double y, double delta);
        void rotate(double around_up_axis, double around_right_axis);
        void move(const vec2& delta);
        void resize(int width, int height);

        void information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width, int* paint_width,
                         int* paint_height) const;
        Information information() const;
        vec3 light_direction() const;
};
