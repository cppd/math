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

#pragma once

#include "chrono.h"

#include <deque>
#include <vector>

namespace ns
{
class Frequency final
{
        struct Events final
        {
                long long sample_number;
                double event_count = 0;

                explicit Events(const int sample_number)
                        : sample_number(sample_number)
                {
                }
        };

        const Clock::time_point start_time_ = Clock::now();
        const int sample_count_;
        const double sample_frequency_;
        const std::vector<double> window_;
        std::deque<Events> deque_;

public:
        Frequency(double interval_length, int sample_count);
        double calculate();
};
}
