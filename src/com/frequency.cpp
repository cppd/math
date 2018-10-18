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

#include "frequency.h"

#include "com/math.h"
#include "com/time.h"

namespace
{
template <size_t N>
std::array<double, N> window_function()
{
        // Richard G. Lyons.
        // Understanding Digital Signal Processing. Third Edition.
        // Pearson Education, Inc. 2011.
        //
        // 5.3.2 Windows Used in FIR Filter Design.
        // Blackman window function.

        std::array<double, N> array;

        double sum = 0;
        for (size_t i = 1; i < N + 1; ++i)
        {
                double x = static_cast<double>(i) / (N + 1);
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
}

Frequency::Frequency() : m_window(window_function<decltype(m_window)().size()>())
{
}

double Frequency::calculate()
{
        const int sample_number = time_in_seconds() * (SAMPLE_COUNT / INTERVAL_LENGTH);

        while (!m_deque.empty() && (m_deque.front().sample_number < sample_number - SAMPLE_COUNT))
        {
                m_deque.pop_front();
        }

        for (int i = SAMPLE_COUNT - m_deque.size(); i >= 0; --i)
        {
                m_deque.emplace_back(sample_number - i);
        }

        ++(m_deque.back().event_count);

        double sum = 0;
        for (int i = 0; i < SAMPLE_COUNT; ++i)
        {
                sum += m_window[i] * m_deque[i].event_count;
        }

        return sum * (SAMPLE_COUNT / INTERVAL_LENGTH);
}
