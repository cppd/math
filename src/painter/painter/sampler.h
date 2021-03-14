/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/sampling/halton_sampler.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Sampler final
{
        sampling::HaltonSampler<N, T> m_sampler;
        std::vector<Vector<N, T>> m_samples;
        int m_samples_per_pixel;

        void generate_samples()
        {
                m_samples.clear();
                for (int i = 0; i < m_samples_per_pixel; ++i)
                {
                        m_samples.push_back(m_sampler.generate());
                }
        }

public:
        explicit Sampler(int samples_per_pixel) : m_samples_per_pixel(samples_per_pixel)
        {
                if (samples_per_pixel <= 0)
                {
                        error("Painter samples per pixel " + to_string(samples_per_pixel) + " is negative");
                }

                generate_samples();
        }

        void generate(std::vector<Vector<N, T>>* samples) const
        {
                *samples = m_samples;
        }

        void next_pass()
        {
                generate_samples();
        }
};
}
