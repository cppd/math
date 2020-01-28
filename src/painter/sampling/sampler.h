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

#include "engine.h"

#include "com/error.h"
#include "com/math.h"
#include "com/names.h"

template <size_t N, typename T>
class StratifiedJitteredSampler
{
        StratifiedJitteredSampleEngine<N, T> m_engine;

        static int one_dimension_size(int sample_count)
        {
                if (sample_count < 1)
                {
                        error("Stratified jittered sample count (" + to_string(sample_count) +
                              ") is not a positive integer");
                }

                double v = std::pow(sample_count, 1.0 / N);

                if (unsigned v_floor = std::floor(v); power<N>(v_floor) >= static_cast<unsigned>(sample_count))
                {
                        return v_floor;
                }

                if (unsigned v_ceil = std::ceil(v); power<N>(v_ceil) >= static_cast<unsigned>(sample_count))
                {
                        return v_ceil;
                }

                error("Could not compute one dimension sample count for " + to_string(sample_count) + " samples in " +
                      space_name(N));
        }

public:
        explicit StratifiedJitteredSampler(int sample_count) : m_engine(one_dimension_size(sample_count))
        {
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                m_engine.generate(random_engine, samples);
        }
};

template <size_t N, typename T>
class LatinHypercubeSampler
{
        LatinHypercubeSampleEngine<N, T> m_engine;

public:
        explicit LatinHypercubeSampler(int sample_count) : m_engine(sample_count)
        {
        }

        template <typename RandomEngine>
        void generate(RandomEngine& random_engine, std::vector<Vector<N, T>>* samples) const
        {
                m_engine.generate(random_engine, samples);
        }
};
