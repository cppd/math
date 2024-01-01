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

/*
Richard G. Lyons.
Understanding Digital Signal Processing. Third Edition.
Pearson Education, Inc. 2011.

5.3.2 Windows Used in FIR Filter Design
Blackman window function
*/

#include "frequency.h"

#include "chrono.h"
#include "constant.h"
#include "error.h"

#include <cmath>
#include <vector>

namespace ns
{
namespace
{
std::vector<double> lowpass_filter_window_function(const int tap_count)
{
        if (tap_count < 1)
        {
                error("Lowpass filter tap count < 1");
        }

        std::vector<double> window(tap_count);

        double sum = 0;
        for (int i = 1; i < tap_count + 1; ++i)
        {
                const double x = static_cast<double>(i) / (tap_count + 1);
                const double v = 0.42 - 0.5 * std::cos(2 * PI<double> * x) + 0.08 * std::cos(4 * PI<double> * x);
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

Frequency::Frequency(const double interval_length, const int sample_count)
        : sample_count_(sample_count),
          sample_frequency_(sample_count_ / interval_length),
          window_(lowpass_filter_window_function(sample_count_))
{
        if (interval_length <= 0)
        {
                error("Filter interval length <= 0");
        }
}

double Frequency::calculate()
{
        const auto sample_number = static_cast<long long>(duration_from(start_time_) * sample_frequency_);
        const long long min_sample_number = sample_number - sample_count_;

        while (!deque_.empty() && (deque_.front().sample_number < min_sample_number))
        {
                deque_.pop_front();
        }

        ASSERT(deque_.size() <= 1u + sample_count_);

        const long long end_sample_number = min_sample_number + deque_.size();

        ASSERT(deque_.empty()
               || (deque_.front().sample_number == min_sample_number
                   && deque_.back().sample_number + 1 == end_sample_number));

        for (long long i = end_sample_number; i <= sample_number; ++i)
        {
                deque_.emplace_back(i);
        }

        ASSERT(deque_.size() == 1u + sample_count_);

        ++(deque_.back().event_count);

        double sum = 0;
        for (int i = 0; i < sample_count_; ++i)
        {
                sum += window_[i] * deque_[i].event_count;
        }

        return sum * sample_frequency_;
}
}
