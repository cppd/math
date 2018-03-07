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

#include "objects.h"

#include "path_tracing/projectors/projector.h"

class VisiblePerspectiveProjector final : public Projector
{
        PerspectiveProjector<3, double> m_projector;

public:
        VisiblePerspectiveProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                                    double width_view_angle_degrees, const std::array<int, 2>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, width_view_angle_degrees, screen_size)
        {
        }

        int screen_width() const override
        {
                return m_projector.screen_size()[0];
        }

        int screen_height() const override
        {
                return m_projector.screen_size()[1];
        }

        ray3 ray(const vec2& point) const override
        {
                return m_projector.ray(point);
        }
};

class VisibleParallelProjector final : public Projector
{
        ParallelProjector<3, double> m_projector;

public:
        VisibleParallelProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                                 double view_width, const std::array<int, 2>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, view_width, screen_size)
        {
        }

        int screen_width() const override
        {
                return m_projector.screen_size()[0];
        }

        int screen_height() const override
        {
                return m_projector.screen_size()[1];
        }

        ray3 ray(const vec2& point) const override
        {
                return m_projector.ray(point);
        }
};

class VisibleSphericalProjector final : public Projector
{
        SphericalProjector<3, double> m_projector;

public:
        VisibleSphericalProjector(const vec3& camera_org, const vec3& camera_dir, const std::array<vec3, 2>& screen_axes,
                                  double width_view_angle_degrees, const std::array<int, 2>& screen_size)
                : m_projector(camera_org, camera_dir, screen_axes, width_view_angle_degrees, screen_size)
        {
        }

        int screen_width() const override
        {
                return m_projector.screen_size()[0];
        }

        int screen_height() const override
        {
                return m_projector.screen_size()[1];
        }

        ray3 ray(const vec2& point) const override
        {
                return m_projector.ray(point);
        }
};
