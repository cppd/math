/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/frequency.h>
#include <src/text/text_data.h>

namespace ns::view::com
{
class FrameRate final
{
        Frequency frequency_;
        text::TextData text_data_;

public:
        FrameRate();

        void set_text_size(unsigned text_size_in_pixels);

        void calculate();

        [[nodiscard]] const text::TextData& text_data() const
        {
                return text_data_;
        }
};
}
