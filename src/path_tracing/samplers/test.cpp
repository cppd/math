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

#include "test.h"

#include "com/log.h"
#include "com/print.h"
#include "com/random.h"
#include "com/time.h"
#include "path_tracing/samplers/sampler.h"

void test_samplers()
{
        constexpr int iter_count = 1e6;
        constexpr int sample_count = 25;

        {
                std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
                std::vector<vec2> data(sample_count);

                double t = get_time_seconds();

                for (int i = 0; i < iter_count; ++i)
                {
                        StratifiedJitteredSampler(sample_count).generate(random_engine, &data);
                }

                LOG("Stratified Jittered Sampler: time = " + to_string_fixed(get_time_seconds() - t, 5) +
                    " seconds, size = " + to_string(data.size()));

                for (unsigned i = 0; i < data.size(); ++i)
                {
                        LOG("  " + to_string(data[i]));
                }
        }

        {
                std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
                std::vector<vec2> data(sample_count);

                double t = get_time_seconds();

                for (int i = 0; i < iter_count; ++i)
                {
                        LatinHypercubeSampler(sample_count).generate(random_engine, &data);
                }

                LOG("Latin Hypercube Sampler: time = " + to_string_fixed(get_time_seconds() - t, 5) +
                    " seconds, size = " + to_string(data.size()));

                for (unsigned i = 0; i < data.size(); ++i)
                {
                        LOG("  " + to_string(data[i]));
                }
        }
}
