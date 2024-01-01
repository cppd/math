/*
Copyright (C) 2017-2024 Topological Manifold

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

class Font
{
        [[nodiscard]] virtual std::optional<Char> render_impl(char32_t code_point) const = 0;

public:
        virtual ~Font() = default;

        virtual void set_size(int size_in_pixels) = 0;

        template <typename T>
        [[nodiscard]] std::optional<Char> render(const T code_point) const
        {
                static_assert(std::is_same_v<T, char32_t>);
                return render_impl(code_point);
        }

        virtual void render_ascii_printable_characters_to_files() const = 0;
};

[[nodiscard]] std::unique_ptr<Font> create_font(int size_in_pixels, std::vector<unsigned char>&& font_data);
}
