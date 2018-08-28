/*
Copyright (C) 2017, 2018 Topological Manifold

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

        vec3 m_view_center;
        double m_view_width;

        int m_paint_width = -1;
        int m_paint_height = -1;

        void set_vectors(const vec3& right, const vec3& up);

public:
        Camera();

        void set(const vec3& right, const vec3& up);
        void get(vec3* camera_up, vec3* camera_direction, vec3* light_up, vec3* light_direction) const;
        void camera_information(vec3* camera_up, vec3* camera_direction, vec3* view_center, double* view_width, int* paint_width,
                                int* paint_height) const;
        vec3 light_direction() const;
        void rotate(double around_up_axis, double around_right_axis);
        void set_view_center_and_width(const vec3& vec, double view_width, int paint_width, int paint_height);
};
