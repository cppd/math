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

#include "discrepancy.h"

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
                "samples_" + std::string(short_sampler_name(sampler)) + "_" + (sampler.shuffled() ? "shuffled_" : "")
                + to_string(N) + "d_" + replace_space(type_name<T>()) + ".txt");
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
void write_to_file(RandomEngine& random_engine, const Sampler& sampler, int pass_count)
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
void write_to_files(bool shuffle)
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int pass_count = 10;

        LOG(std::string("Writing samples, ") + (shuffle ? "shuffle, " : "") + to_string(N) + "D");
        write_to_file<N, T>(
                random_engine, StratifiedJitteredSampler<N, T>(0, 1, sample_count<N>(), shuffle), pass_count);
        write_to_file<N, T>(random_engine, LatinHypercubeSampler<N, T>(0, 1, sample_count<N>(), shuffle), pass_count);
}

template <std::size_t N, typename T, typename RandomEngine>
void write_to_files()
{
        write_to_files<N, T, RandomEngine>(false);
        write_to_files<N, T, RandomEngine>(true);
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance(bool shuffle)
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int iter_count = 1e6;

        LOG(std::string("Testing performance, ") + (shuffle ? "shuffle, " : "") + to_string(N) + "D");
        test_performance<N, T>(
                random_engine, StratifiedJitteredSampler<N, T>(0, 1, sample_count<N>(), shuffle), iter_count);
        test_performance<N, T>(
                random_engine, LatinHypercubeSampler<N, T>(0, 1, sample_count<N>(), shuffle), iter_count);
        LOG("");
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        test_performance<N, T, RandomEngine>(false);
        test_performance<N, T, RandomEngine>(true);
}

template <typename T, typename RandomEngine>
void write_to_files()
{
        static_assert(std::is_floating_point_v<T>);

        LOG(std::string("Files <") + type_name<T>() + ", " + random_engine_name<RandomEngine>() + ">");

        write_to_files<2, T, RandomEngine>();
        write_to_files<3, T, RandomEngine>();
        write_to_files<4, T, RandomEngine>();
}

template <typename T, typename RandomEngine>
void test_performance()
{
        static_assert(std::is_floating_point_v<T>);

        LOG(std::string("Performance <") + type_name<T>() + ", " + random_engine_name<RandomEngine>() + ">");
        LOG("");

        test_performance<2, T, RandomEngine>();
        test_performance<3, T, RandomEngine>();
        test_performance<4, T, RandomEngine>();
        test_performance<5, T, RandomEngine>();
        test_performance<6, T, RandomEngine>();
}

template <typename RandomEngine>
void write_to_files()
{
        write_to_files<float, RandomEngine>();
        LOG("");
        write_to_files<double, RandomEngine>();
        LOG("");
        write_to_files<long double, RandomEngine>();
}

template <typename T>
void test_performance()
{
        test_performance<T, std::mt19937>();
        LOG("");
        test_performance<T, std::mt19937_64>();
}

void test_sampler_performance()
{
        write_to_files<std::mt19937_64>();

        LOG("");
        test_performance<float>();
        LOG("");
        test_performance<double>();
        LOG("");
        test_performance<long double>();
}

template <std::size_t N, typename T, typename Sampler, typename RandomEngine>
void test_discrepancy(const Sampler& sampler, T discrepancy_limit, RandomEngine& random_engine)
{
        LOG(std::string(sampler_name(sampler)) + ", " + to_string(N) + "d, " + type_name<T>());

        std::vector<Vector<N, T>> data;
        sampler.generate(random_engine, &data);

        const int BOX_COUNT = 10000;

        T discrepancy = compute_discrepancy(sampler.min(), sampler.max(), data, BOX_COUNT, random_engine);
        LOG("discrepancy = " + to_string(discrepancy));
        if (!(discrepancy < discrepancy_limit))
        {
                error(std::string(sampler_name(sampler)) + " discrepancy " + to_string(discrepancy)
                      + " is out of discrepancy limit " + to_string(discrepancy_limit));
        }
}

template <typename T, typename RandomEngine>
std::array<T, 2> min_max_for_samplers(RandomEngine& random_engine)
{
        T min;
        T max;
        if (std::bernoulli_distribution(0.5)(random_engine))
        {
                min = std::uniform_real_distribution<T>(-10, 10)(random_engine);
                max = std::uniform_real_distribution<T>(min + 0.1, min + 10)(random_engine);
        }
        else if (std::bernoulli_distribution(0.5)(random_engine))
        {
                min = 0;
                max = 1;
        }
        else
        {
                min = -1;
                max = 1;
        }
        return {min, max};
}

template <std::size_t N, typename T>
void test_discrepancy(int sample_count, T discrepancy_limit)
{
        std::mt19937 engine = create_engine<std::mt19937>();

        const auto [min, max] = min_max_for_samplers<T>(engine);
        LOG("Sampler min = " + to_string(min) + ", max = " + to_string(max));

        {
                StratifiedJitteredSampler<N, T> sampler(min, max, sample_count, true);
                test_discrepancy<N, T>(sampler, discrepancy_limit, engine);
        }
        {
                LatinHypercubeSampler<N, T> sampler(min, max, sample_count, true);
                test_discrepancy<N, T>(sampler, discrepancy_limit, engine);
        }
}

template <std::size_t N>
void test_discrepancy(int sample_count, double discrepancy_limit)
{
        test_discrepancy<N, float>(sample_count, discrepancy_limit);
        test_discrepancy<N, double>(sample_count, discrepancy_limit);
}

void test_sampler_discrepancy()
{
        LOG("Test sampler discrepancy");

        test_discrepancy<2>(power<2>(10), 0.13);
        test_discrepancy<3>(power<3>(10), 0.04);
        test_discrepancy<4>(power<4>(10), 0.012);
}

TEST_SMALL("Sampler discrepancy", test_sampler_discrepancy)
TEST_PERFORMANCE("Samplers", test_sampler_performance)
}
}
