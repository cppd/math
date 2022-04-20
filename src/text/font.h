/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <memory>
#include <optional>
#include <vector>

namespace ns::text
{
class Font final
{
        class Impl;
        std::unique_ptr<Impl> impl_;

public:
        Font(int size_in_pixels, std::vector<unsigned char>&& font_data);
        ~Font();

        void set_size(int size_in_pixels);

        struct Char final
        {
                const unsigned char* image;
                int size;
                int width;
                int height;
                int left;
                int top;
                int advance_x;
                char32_t code_point;
        };

        template <typename T>
        std::optional<Char> render(T code_point) const;
};
}
