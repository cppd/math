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

#pragma once

#include "sample_generators.h"

#include "com/error.h"
#include "path_tracing/objects.h"

template <size_t N, typename T>
class StratifiedJitteredSampler final : public Sampler<N, T>
{
        const int m_samples_one_dimension;

public:
        StratifiedJitteredSampler(int sample_count) : m_samples_one_dimension(std::ceil(std::pow(sample_count, 1.0 / N)))
        {
                if (sample_count < 1)
                {
                        error("Stratified jittered sample count (" + to_string(sample_count) + ") is not a positive integer");
                }

                ASSERT(m_samples_one_dimension > 0);
                ASSERT(std::pow(m_samples_one_dimension, N) >= sample_count);
        }

        void generate(std::mt19937_64& random_engine, std::vector<Vector<N, T>>* samples) const override
        {
                stratified_jittered_samples(random_engine, m_samples_one_dimension, samples);
        }
};

template <size_t N, typename T>
class LatinHypercubeSampler final : public Sampler<N, T>
{
        const int m_sample_count;

public:
        LatinHypercubeSampler(int sample_count) : m_sample_count(sample_count)
        {
                if (sample_count < 1)
                {
                        error("Latin hypercube sample count (" + to_string(sample_count) + ") is not a positive integer");
                }
        }

        void generate(std::mt19937_64& random_engine, std::vector<Vector<N, T>>* samples) const override
        {
                latin_hypercube_samples(random_engine, m_sample_count, samples);
        }
};
