/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "com/conversion.h"
#include "com/print.h"

#include <cmath>

constexpr double INTERVAL_LENGTH = 1;
constexpr int SAMPLE_COUNT = 10;

constexpr double TEXT_SIZE_IN_POINTS = 9.0;
constexpr double TEXT_STEP_Y_IN_POINTS = 1.3 * TEXT_SIZE_IN_POINTS;
constexpr double TEXT_X_IN_POINTS = 5;
constexpr double TEXT_Y_IN_POINTS = TEXT_STEP_Y_IN_POINTS;
constexpr const char* TEXT = "FPS: ";

FrameRate::FrameRate(double ppi) : m_frequency(INTERVAL_LENGTH, SAMPLE_COUNT)
{
        m_text_size = points_to_pixels(TEXT_SIZE_IN_POINTS, ppi);

        m_text_data.step_y = points_to_pixels(TEXT_STEP_Y_IN_POINTS, ppi);
        m_text_data.start_x = points_to_pixels(TEXT_X_IN_POINTS, ppi);
        m_text_data.start_y = points_to_pixels(TEXT_Y_IN_POINTS, ppi);
        m_text_data.text.resize(2);
        m_text_data.text[0] = TEXT;
        m_text_data.text[1] = "";
}

void FrameRate::calculate()
{
        m_text_data.text[1] = to_string(std::lround(m_frequency.calculate()));
}
