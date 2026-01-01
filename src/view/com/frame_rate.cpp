/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "frame_rate.h"

#include <src/com/print.h>

#include <cmath>

namespace ns::view::com
{
namespace
{
constexpr double INTERVAL_LENGTH = 1;
constexpr int SAMPLE_COUNT = 10;
constexpr const char* TEXT = "FPS: ";
}

FrameRate::FrameRate()
        : frequency_(INTERVAL_LENGTH, SAMPLE_COUNT)
{
        text_data_.text.resize(2);
        text_data_.text[0] = TEXT;
        text_data_.text[1] = "";
}

void FrameRate::set_text_size(const unsigned text_size_in_pixels)
{
        text_data_.step_y = std::lround(1.3 * text_size_in_pixels);
        text_data_.start_x = std::lround(0.5 * text_size_in_pixels);
        text_data_.start_y = text_data_.step_y;
}

void FrameRate::calculate()
{
        text_data_.text[1] = to_string(std::lround(frequency_.calculate()));
}
}
