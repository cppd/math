/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/error.h"
#include "com/math.h"
#include "com/time.h"

namespace
{
// Richard G. Lyons.
// Understanding Digital Signal Processing. Third Edition.
// Pearson Education, Inc. 2011.
//
// 5.3.2 Windows Used in FIR Filter Design.
// Blackman window function.
std::vector<double> lowpass_filter_window_function(size_t tap_count)
{
        if (tap_count < 1)
        {
                error("Lowpass filter tap count < 1");
        }

        std::vector<double> window(tap_count);

        double sum = 0;
        for (size_t i = 1; i < tap_count + 1; ++i)
        {
                double x = static_cast<double>(i) / (tap_count + 1);
                double v = 0.42 - 0.5 * std::cos(2 * PI<double> * x) + 0.08 * std::cos(4 * PI<double> * x);
                window[i - 1] = v;
                sum += v;
        }

        for (auto& v : window)
        {
                v /= sum;
        }

        return window;
}
}

Frequency::Frequency(double interval_length, int sample_count)
        : m_interval_length(interval_length),
          m_sample_count(sample_count),
          m_sample_frequency(m_sample_count / m_interval_length),
          m_window(lowpass_filter_window_function(m_sample_count))
{
        if (interval_length <= 0)
        {
                error("Filter interval length <= 0");
        }
}

double Frequency::calculate()
{
        const int sample_number = time_in_seconds() * m_sample_frequency;

        while (!m_deque.empty() && (m_deque.front().sample_number < sample_number - m_sample_count))
        {
                m_deque.pop_front();
        }

        for (int i = m_sample_count - m_deque.size(); i >= 0; --i)
        {
                m_deque.emplace_back(sample_number - i);
        }

        ASSERT(m_deque.size() == 1u + m_sample_count);

        ++(m_deque.back().event_count);

        double sum = 0;
        for (int i = 0; i < m_sample_count; ++i)
        {
                sum += m_window[i] * m_deque[i].event_count;
        }

        return sum * m_sample_frequency;
}
