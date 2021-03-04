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

#include "../lh_sampler.h"
#include "../sj_sampler.h"

#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <random>
#include <string_view>

namespace ns::sampling::test
{
namespace
{
std::string replace_space(const std::string_view& s)
{
        std::string r;
        r.reserve(s.size());
        for (char c : s)
        {
                r += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return r;
}

template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::mt19937>, const char*> random_engine_name()
{
        return "std::mt19937";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::mt19937_64>, const char*> random_engine_name()
{
        return "std::mt19937_64";
}

template <std::size_t N, typename T>
const std::string_view& sampler_name(const StratifiedJitteredSampler<N, T>&)
{
        static constexpr std::string_view s = "Stratified Jittered Sampler";
        return s;
}

template <std::size_t N, typename T>
const std::string_view& sampler_name(const LatinHypercubeSampler<N, T>&)
{
        static constexpr std::string_view s = "Latin Hypercube Sampler";
        return s;
}

template <std::size_t N, typename T>
const std::string_view& short_sampler_name(const StratifiedJitteredSampler<N, T>&)
{
        static constexpr std::string_view s = "sjs";
        return s;
}

template <std::size_t N, typename T>
const std::string_view& short_sampler_name(const LatinHypercubeSampler<N, T>&)
{
        static constexpr std::string_view s = "lhc";
        return s;
}

template <template <std::size_t, typename> typename S, std::size_t N, typename T>
std::filesystem::path sampler_file_name(const S<N, T>& sampler)
{
        return path_from_utf8(
                "samples_" + std::string(short_sampler_name(sampler)) + "_" + to_string(N) + "d_"
                + replace_space(type_name<T>()) + ".txt");
}

template <std::size_t N>
constexpr int sample_count()
{
        static_assert(N >= 2);
        switch (N)
        {
        case 2:
        case 3:
                return power<N>(5u);
        case 4:
                return power<N>(4u);
        case 5:
        case 6:
                return power<N>(3u);
        default:
                return power<N>(2u);
        }
}

template <std::size_t N, typename T, typename Sampler, typename RandomEngine>
void write_samples_to_file(RandomEngine& random_engine, const Sampler& sampler, int pass_count)
{
        std::ofstream file(std::filesystem::temp_directory_path() / sampler_file_name(sampler));

        file << sampler_name(sampler) << "\n";
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

template <std::size_t N, typename T, typename Sampler, typename RandomEngine>
void test_performance(RandomEngine& random_engine, const Sampler& sampler, int iter_count)
{
        std::vector<Vector<N, T>> data;

        TimePoint start_time = time();

        for (int i = 0; i < iter_count; ++i)
        {
                sampler.generate(random_engine, &data);
        }

        LOG(std::string(sampler_name(sampler)) + ": time = " + to_string_fixed(duration_from(start_time), 5)
            + " seconds, size = " + to_string(data.size()));
}

template <std::size_t N, typename T, typename RandomEngine>
void write_samples_to_files()
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int pass_count = 10;

        LOG("Writing samples " + to_string(N) + "D");
        write_samples_to_file<N, T>(random_engine, StratifiedJitteredSampler<N, T>(sample_count<N>()), pass_count);
        write_samples_to_file<N, T>(random_engine, LatinHypercubeSampler<N, T>(sample_count<N>()), pass_count);
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int iter_count = 1e6;

        LOG("Testing performance " + to_string(N) + "D");
        test_performance<N, T>(random_engine, StratifiedJitteredSampler<N, T>(sample_count<N>()), iter_count);
        test_performance<N, T>(random_engine, LatinHypercubeSampler<N, T>(sample_count<N>()), iter_count);
}

template <typename T, typename RandomEngine>
void write_samples_to_files()
{
        static_assert(std::is_floating_point_v<T>);

        LOG(std::string("Files <") + type_name<T>() + ", " + random_engine_name<RandomEngine>() + ">");

        write_samples_to_files<2, T, RandomEngine>();
        write_samples_to_files<3, T, RandomEngine>();
        write_samples_to_files<4, T, RandomEngine>();
}

template <typename T, typename RandomEngine>
void test_performance()
{
        static_assert(std::is_floating_point_v<T>);

        LOG(std::string("Performance <") + type_name<T>() + ", " + random_engine_name<RandomEngine>() + ">");

        test_performance<2, T, RandomEngine>();
        test_performance<3, T, RandomEngine>();
        test_performance<4, T, RandomEngine>();
        test_performance<5, T, RandomEngine>();
        test_performance<6, T, RandomEngine>();
}

template <typename RandomEngine>
void write_samples_to_files()
{
        write_samples_to_files<float, RandomEngine>();
        LOG("");
        write_samples_to_files<double, RandomEngine>();
        LOG("");
        write_samples_to_files<long double, RandomEngine>();
}

template <typename T>
void test_performance()
{
        test_performance<T, std::mt19937>();
        LOG("");
        test_performance<T, std::mt19937_64>();
}

void test()
{
        write_samples_to_files<std::mt19937_64>();

        LOG("");
        test_performance<float>();
        LOG("");
        test_performance<double>();
        LOG("");
        test_performance<long double>();
}

TEST_PERFORMANCE("Samplers", test)
}
}
