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

#include "com/vec.h"

#include <string>
#include <vector>

class Image
{
        std::vector<vec3> m_data;

        // тип long long вместо int нужен для умножений
        long long m_width, m_height;
        double m_max_x, m_max_y;
        int m_max_x0, m_max_y0;

        void read_from_srgba_pixels(int width, int height, const unsigned char* srgba_pixels);

public:
        Image(int width, int height);

        Image(int width, int height, const std::vector<unsigned char>& srgba_pixels);

        void resize(int width, int height);

        int width() const;
        int height() const;

        bool empty() const;

        void clear(const vec3& color = vec3(0));

        void set_pixel(int x, int y, const vec3& color);
        const vec3& get_pixel(int x, int y) const;

        vec3 get_texture(const vec2& p) const;

        void read_from_file(const std::string& file_name);
        void write_to_file(const std::string& file_name) const;

        void flip_vertically();
};
