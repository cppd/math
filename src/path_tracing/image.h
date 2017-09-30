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

#include "vec3.h"

#include <string>
#include <vector>

class Image
{
        std::vector<vec3> m_data;

        int m_width, m_height;
        double m_max_x, m_max_y;
        int m_max_x0, m_max_y0;

public:
        Image();

        Image(int width, int height);

        void resize(int width, int height);

        int width() const;
        int height() const;

        bool empty() const;

        void clear(const vec3& color = vec3(0));

        void set_pixel(int x, int y, const vec3& color);
        const vec3& get_pixel(int x, int y) const;

        vec3 get_texture(double x, double y) const;

        void read_from_file(const std::string& file_name);
        void write_to_file(const std::string& file_name) const;

        void flip_vertically();
};
