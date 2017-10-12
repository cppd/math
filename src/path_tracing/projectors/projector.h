/*
Copyright (C) 2017 Topological Manifold

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

#include "path_tracing/objects.h"

#include "com/error.h"
#include "com/math.h"

class PerspectiveProjector final : public Projector
{
        int m_width, m_height, m_pixel_resolution;
        vec2 m_screen_org;
        vec3 m_org, m_dir, m_x, m_y;

public:
        PerspectiveProjector(const vec3& camera_org, const vec3& camera_dir, const vec3& camera_up,
                             double width_view_angle_degrees, int width, int height, int pixel_resolution)
                : m_width(width), m_height(height), m_pixel_resolution(pixel_resolution), m_screen_org(-width * 0.5, height * 0.5)
        {
                if (!(width_view_angle_degrees > 0 && width_view_angle_degrees < 180))
                {
                        error("Perspective projection: error view angle " + to_string(width_view_angle_degrees));
                }
                if (width <= 0 || height <= 0 || pixel_resolution <= 0)
                {
                        error("Perspective projection: error (width, height, pixel resolution) (" + to_string(width) + ", " +
                              to_string(height) + ", " + to_string(pixel_resolution) + ")");
                }

                double half_angle = width_view_angle_degrees * 0.5 / 180.0 * PI;
                double dir_length = width * 0.5 * std::tan(0.5 * PI - half_angle);

                m_org = camera_org;
                m_dir = normalize(camera_dir) * dir_length;
                m_x = normalize(cross(camera_dir, camera_up));
                m_y = normalize(cross(m_x, camera_dir));
        }

        int screen_width() const override
        {
                return m_width;
        }

        int screen_height() const override
        {
                return m_height;
        }

        int pixel_resolution() const override
        {
                return m_pixel_resolution;
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 p = vec2(m_screen_org[0] + point[0], m_screen_org[1] - point[1]);
                return ray3(m_org, m_dir + m_x * p[0] + m_y * p[1]);
        }
};

class ParallelProjector final : public Projector
{
        int m_width, m_height, m_pixel_resolution;
        vec2 m_screen_org;
        vec3 m_org, m_dir, m_x, m_y;

public:
        ParallelProjector(const vec3& camera_org, const vec3& camera_dir, const vec3& camera_up, double view_width, int width,
                          int height, int pixel_resolution)
                : m_width(width), m_height(height), m_pixel_resolution(pixel_resolution), m_screen_org(-width * 0.5, height * 0.5)
        {
                if (!(view_width > 0))
                {
                        error("Error view width for parallel projection");
                }
                if (width <= 0 || height <= 0 || pixel_resolution <= 0)
                {
                        error("Parallel projection: error (width, height, pixel resolution) (" + to_string(width) + ", " +
                              to_string(height) + ", " + to_string(pixel_resolution) + ")");
                }

                m_org = camera_org;
                m_dir = normalize(camera_dir);

                double coef = view_width / width;
                m_x = coef * normalize(cross(camera_dir, camera_up));
                m_y = coef * normalize(cross(m_x, camera_dir));
        }

        int screen_width() const override
        {
                return m_width;
        }

        int screen_height() const override
        {
                return m_height;
        }

        int pixel_resolution() const override
        {
                return m_pixel_resolution;
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 p = vec2(m_screen_org[0] + point[0], m_screen_org[1] - point[1]);
                return ray3(m_org + m_x * p[0] + m_y * p[1], m_dir);
        }
};

// Параллельное проецирование точек экрана на полусферу и создание лучей
// из центра полусферы в направлении точек на сфере.
class SphericalProjector final : public Projector
{
        int m_width, m_height, m_pixel_resolution;
        vec2 m_screen_org;
        vec3 m_org, m_dir, m_x, m_y;
        double m_square_radius;

public:
        SphericalProjector(const vec3& camera_org, const vec3& camera_dir, const vec3& camera_up, double width_view_angle_degrees,
                           int width, int height, int pixel_resolution)
                : m_width(width), m_height(height), m_pixel_resolution(pixel_resolution), m_screen_org(-width * 0.5, height * 0.5)
        {
                double half_angle = width_view_angle_degrees * 0.5 / 180.0 * PI;

                double sin_alpha = std::sin(half_angle);

                if (!(width_view_angle_degrees > 0 && (square(sin_alpha) + square(sin_alpha / width * height) < 1)))
                {
                        error("Error view angle for spherical projection");
                }
                if (width <= 0 || height <= 0 || pixel_resolution <= 0)
                {
                        error("Spherical projection: error (width, height, pixel resolution) (" + to_string(width) + ", " +
                              to_string(height) + ", " + to_string(pixel_resolution) + ")");
                }

                m_square_radius = square(width * 0.5 / sin_alpha);

                m_org = camera_org;
                m_dir = normalize(camera_dir);
                m_x = normalize(cross(camera_dir, camera_up));
                m_y = normalize(cross(m_x, camera_dir));
        }

        int screen_width() const override
        {
                return m_width;
        }

        int screen_height() const override
        {
                return m_height;
        }

        int pixel_resolution() const override
        {
                return m_pixel_resolution;
        }

        ray3 ray(const vec2& point) const override
        {
                vec2 p = vec2(m_screen_org[0] + point[0], m_screen_org[1] - point[1]);
                double radicand = m_square_radius - square(p[0]) - square(p[1]);
                if (radicand <= 0)
                {
                        error("Error spherical projection radicand");
                }
                double z = std::sqrt(radicand);
                return ray3(m_org, m_dir * z + m_x * p[0] + m_y * p[1]);
        }
};
