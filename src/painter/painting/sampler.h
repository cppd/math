/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/numerical/vector.h>
#include <src/sampling/halton_sampler.h>
#include <src/sampling/sj_sampler.h>

#include <cstddef>
#include <vector>

namespace ns::painter::painting
{
template <std::size_t N, typename T>
class SamplerHalton final
{
        sampling::HaltonSampler<N, T> sampler_;
        std::vector<Vector<N, T>> samples_;
        int samples_per_pixel_;

        void generate_samples()
        {
                samples_.clear();
                for (int i = 0; i < samples_per_pixel_; ++i)
                {
                        samples_.push_back(sampler_.generate());
                }
        }

public:
        explicit SamplerHalton(const int samples_per_pixel)
                : samples_per_pixel_(samples_per_pixel)
        {
                if (samples_per_pixel <= 0)
                {
                        error("Painter samples per pixel " + to_string(samples_per_pixel) + " is negative");
                }

                generate_samples();
        }

        template <typename RandomEngine>
        void generate(RandomEngine&, std::vector<Vector<N, T>>* const samples) const
        {
                *samples = samples_;
        }

        void next_pass()
        {
                generate_samples();
        }
};

template <std::size_t N, typename T>
class SamplerStratifiedJittered final
{
        static constexpr T MIN = 0;
        static constexpr T MAX = 1;
        static constexpr bool SHUFFLE = false;

        sampling::StratifiedJitteredSampler<N, T> sampler_;

public:
        explicit SamplerStratifiedJittered(const int samples_per_pixel)
                : sampler_(MIN, MAX, samples_per_pixel, SHUFFLE)
        {
        }

        template <typename RandomEngine>
        void generate(RandomEngine& engine, std::vector<Vector<N, T>>* const samples) const
        {
                sampler_.generate(engine, samples);
        }

        void next_pass() const
        {
        }
};
}
