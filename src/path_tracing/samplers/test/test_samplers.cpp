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

#include "test_samplers.h"

#include "com/log.h"
#include "com/print.h"
#include "com/random.h"
#include "com/time.h"
#include "path_tracing/samplers/sampler.h"

namespace
{
template <typename Sampler, typename RandomEngine>
void generate_points(RandomEngine& random_engine, int sample_count, int pass_count)
{
        std::vector<vec2> data(sample_count);

        std::vector<vec2> all_data;
        all_data.reserve(sample_count * pass_count);

        LOG("Pass count: " + to_string(pass_count));

        for (int i = 0; i < pass_count; ++i)
        {
                Sampler(sample_count).generate(random_engine, &data);
                all_data.insert(all_data.end(), data.begin(), data.end());
        }

        for (const vec2& v : all_data)
        {
                LOG("  " + to_string(v));
        }
}

template <typename Sampler>
void test_sampler(const std::string& name, int iter_count, int sample_count, int pass_count)
{
        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
        std::vector<vec2> data(sample_count);

        double t = time_in_seconds();

        for (int i = 0; i < iter_count; ++i)
        {
                StratifiedJitteredSampler(sample_count).generate(random_engine, &data);
        }

        LOG(name + ": time = " + to_string_fixed(time_in_seconds() - t, 5) + " seconds, size = " + to_string(data.size()));

        generate_points<Sampler>(random_engine, sample_count, pass_count);
}
}

void test_samplers()
{
        constexpr int iter_count = 1e6;
        constexpr int sample_count = 25;
        constexpr int pass_count = 10;

        test_sampler<StratifiedJitteredSampler>("Stratified Jittered Sampler", iter_count, sample_count, pass_count);

        LOG("");

        test_sampler<LatinHypercubeSampler>("Latin Hypercube Sampler", iter_count, sample_count, pass_count);
}
