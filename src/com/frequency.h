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

#pragma once

#include <deque>
#include <vector>

class Frequency
{
        struct Events
        {
                int sample_number;
                double event_count = 0;
                explicit Events(int sample_number_) : sample_number(sample_number_)
                {
                }
        };

        // Интервал в секундах
        const double m_interval_length;
        // Сколько отсчётов на интервале, не считая текущего
        const int m_sample_count;

        const double m_sample_frequency;

        const std::vector<double> m_window;

        std::deque<Events> m_deque;

public:
        Frequency(double interval_length, int sample_count);
        double calculate();
};
