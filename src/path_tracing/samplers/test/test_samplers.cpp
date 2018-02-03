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

#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "path_tracing/samplers/sampler.h"

#include <fstream>

namespace
{
template <size_t N, typename T, typename Sampler, typename RandomEngine>
void write_samples_to_file(RandomEngine& random_engine, const Sampler& sampler, const std::string& sampler_name,
                           const std::string& file_name, int pass_count)
{
        std::ofstream file(file_name);

        file << sampler_name << "\n";
        file << "Pass count: " << to_string(pass_count) << "\n";

        std::vector<Vector<N, T>> data;

        for (int i = 0; i < pass_count; ++i)
        {
                sampler.generate(random_engine, &data);

                for (const Vector<N, T>& v : data)
                {
                        file << to_string(v) << "\n";
                }
        }
}

template <size_t N, typename T, typename Sampler, typename RandomEngine>
void test_speed(RandomEngine& random_engine, const Sampler& sampler, const std::string& sampler_name, int iter_count)
{
        std::vector<Vector<N, T>> data;

        double t = time_in_seconds();

        for (int i = 0; i < iter_count; ++i)
        {
                sampler.generate(random_engine, &data);
        }

        LOG(sampler_name + ": time = " + to_string_fixed(time_in_seconds() - t, 5) +
            " seconds, size = " + to_string(data.size()));
}

template <size_t N, typename T, typename Sampler, typename RandomEngine>
void test_sampler(RandomEngine& random_engine, const Sampler& sampler, const std::string& sampler_name,
                  const std::string& file_name, int iter_count, int pass_count)
{
        write_samples_to_file<N, T>(random_engine, sampler, sampler_name, file_name, pass_count);

        test_speed<N, T>(random_engine, sampler, sampler_name, iter_count);
}

template <size_t N, typename T, typename RandomEngine>
void test_samplers()
{
        RandomEngineWithSeed<RandomEngine> random_engine;

        int iter_count = 1e6;
        int sample_count = std::pow(5, N);
        int pass_count = 10;

        std::string tmp_dir = temp_directory() + "/";
        const std::string dimension_str = to_string(N) + "d";

        test_sampler<N, T>(random_engine, StratifiedJitteredSampler<N, T>(sample_count), "Stratified Jittered Sampler",
                           tmp_dir + "samples_sjs_" + dimension_str + ".txt", iter_count, pass_count);
        LOG("");
        test_sampler<N, T>(random_engine, LatinHypercubeSampler<N, T>(sample_count), "Latin Hypercube Sampler",
                           tmp_dir + "samples_lhc_" + dimension_str + ".txt", iter_count, pass_count);
}
}

void test_samplers()
{
        test_samplers<2, double, std::mt19937_64>();
        LOG("");
        test_samplers<3, double, std::mt19937_64>();
}
