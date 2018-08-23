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

#include "fps.h"

#include "com/math.h"
#include "com/time.h"

std::array<double, FPS::INTERVAL_SAMPLE_COUNT> FPS::window_function()
{
        // Richard G. Lyons.
        // Understanding Digital Signal Processing. Third Edition.
        // Pearson Education, Inc. 2011.
        //
        // 5.3.2 Windows Used in FIR Filter Design.
        // Blackman window function.

        std::array<double, INTERVAL_SAMPLE_COUNT> array;

        double sum = 0;
        for (size_t i = 1; i < INTERVAL_SAMPLE_COUNT + 1; ++i)
        {
                double x = static_cast<double>(i) / (INTERVAL_SAMPLE_COUNT + 1);
                double v = 0.42 - 0.5 * std::cos(2 * PI<double> * x) + 0.08 * std::cos(4 * PI<double> * x);
                array[i - 1] = v;
                sum += v;
        }

        for (auto& v : array)
        {
                v /= sum;
        }

        return array;
}

FPS::FPS() : m_window(window_function())
{
}

long FPS::calculate()
{
        const int time = time_in_seconds() * (INTERVAL_SAMPLE_COUNT / INTERVAL_LENGTH);

        while (!m_deque.empty() && (m_deque.front().time < time - INTERVAL_SAMPLE_COUNT))
        {
                m_deque.pop_front();
        }

        for (int i = INTERVAL_SAMPLE_COUNT - m_deque.size(); i >= 0; --i)
        {
                m_deque.emplace_back(time - i);
        }

        m_deque.back().fps += INTERVAL_SAMPLE_COUNT / INTERVAL_LENGTH;

        double sum = 0;
        for (int i = 0; i < INTERVAL_SAMPLE_COUNT; ++i)
        {
                sum += m_window[i] * m_deque[i].fps;
        }

        return std::lround(sum);
}
