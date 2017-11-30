/*
Copyright (C) 2017 Topological Manifold

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

#include "com/error.h"
#include "com/print.h"
#include "path_tracing/objects.h"

// Matt Pharr, Wenzel Jakob, Greg Humphreys.
// Physically Based Rendering. From theory to implementation. Third edition.
// Elsevier, 2017.
// 7.3 Stratified sampling.
class StratifiedJitteredSampler final : public Sampler
{
        const int m_samples_per_pixel;
        const int m_samples_one_dimension;
        const double m_region_size;

public:
        StratifiedJitteredSampler(int samples_per_pixel)
                : m_samples_per_pixel(samples_per_pixel),
                  m_samples_one_dimension(std::ceil(std::sqrt(m_samples_per_pixel))),
                  m_region_size(1.0 / m_samples_one_dimension)
        {
                if (m_samples_per_pixel < 1)
                {
                        error("Stratified jittered sample count (" + to_string(m_samples_per_pixel) +
                              ") is not a positive integer");
                }

                ASSERT(m_samples_one_dimension > 0 && square(m_samples_one_dimension) >= m_samples_per_pixel);
        }

        void generate(std::mt19937_64& random_engine, std::vector<vec2>* samples) const override
        {
                std::uniform_real_distribution<double> urd(0, m_region_size);

                samples->resize(m_samples_per_pixel);

                double x = 0;
                for (int i = 0, sample_number = 0; i < m_samples_one_dimension; ++i, x += m_region_size)
                {
                        double y = 0;
                        for (int j = 0; j < m_samples_one_dimension; ++j, y += m_region_size, ++sample_number)
                        {
                                (*samples)[sample_number] = vec2(x + urd(random_engine), y + urd(random_engine));
                        }
                }
        }
};
