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

#include "../halton_sampler.h"
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
#include <sstream>
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
std::string sampler_name(const StratifiedJitteredSampler<N, T>&)
{
        return "Stratified Jittered Sampler";
}

template <std::size_t N, typename T>
std::string sampler_name(const LatinHypercubeSampler<N, T>&)
{
        return "Latin Hypercube Sampler";
}

template <std::size_t N, typename T>
std::string sampler_name(const HaltonSampler<N, T>&)
{
        return "Halton Sampler";
}

template <std::size_t N, typename T>
std::filesystem::path sampler_file_name(const StratifiedJitteredSampler<N, T>& sampler)
{
        std::ostringstream oss;
        oss << "sampler_sjs_";
        if (sampler.shuffled())
        {
                oss << "shuffled_";
        }
        oss << to_string(N) << "d_" << replace_space(type_name<T>()) << ".txt";
        return path_from_utf8(oss.str());
}

template <std::size_t N, typename T>
std::filesystem::path sampler_file_name(const LatinHypercubeSampler<N, T>& sampler)
{
        std::ostringstream oss;
        oss << "sampler_lhc_";
        if (sampler.shuffled())
        {
                oss << "shuffled_";
        }
        oss << to_string(N) << "d_" << replace_space(type_name<T>()) << ".txt";
        return path_from_utf8(oss.str());
}

template <std::size_t N, typename T>
std::filesystem::path sampler_file_name(const HaltonSampler<N, T>&)
{
        std::ostringstream oss;
        oss << "sampler_halton_";
        oss << to_string(N) << "d_" << replace_space(type_name<T>()) << ".txt";
        return path_from_utf8(oss.str());
}

template <std::size_t N>
constexpr int one_dimension_sample_count()
{
        static_assert(N >= 2);
        switch (N)
        {
        case 2:
        case 3:
                return 5u;
        case 4:
                return 4u;
        case 5:
        case 6:
                return 3u;
        default:
                return 2u;
        }
}

template <std::size_t N, typename T>
void write_to_file(
        const std::string& name,
        const std::filesystem::path& file_name,
        int grid_size,
        const std::vector<Vector<N, T>>& data)
{
        std::ofstream file(std::filesystem::temp_directory_path() / file_name);

        file << "Name: " << name << "\n";
        file << "Grid: " << grid_size << "\n";

        for (const Vector<N, T>& v : data)
        {
                file << to_string(v) << "\n";
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void write_to_files(bool shuffle)
{
        static_assert(std::is_floating_point_v<T>);

        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int PASS_COUNT = 10;

        constexpr int ONE_DIMENSION_SAMPLE_COUNT = one_dimension_sample_count<N>();
        constexpr int SAMPLE_COUNT = power<N>(ONE_DIMENSION_SAMPLE_COUNT);

        LOG(std::string("Writing samples, ") + (shuffle ? "shuffle, " : "") + type_name<T>() + ", " + to_string(N)
            + "D");

        {
                StratifiedJitteredSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                std::vector<Vector<N, T>> data;
                std::vector<Vector<N, T>> tmp;
                for (int i = 0; i < PASS_COUNT; ++i)
                {
                        sampler.generate(random_engine, &tmp);
                        data.insert(data.cend(), tmp.cbegin(), tmp.cend());
                }
                constexpr int GRID_SIZE = ONE_DIMENSION_SAMPLE_COUNT;
                write_to_file(sampler_name(sampler), sampler_file_name(sampler), GRID_SIZE, data);
        }
        {
                LatinHypercubeSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                std::vector<Vector<N, T>> data;
                std::vector<Vector<N, T>> tmp;
                for (int i = 0; i < PASS_COUNT; ++i)
                {
                        sampler.generate(random_engine, &tmp);
                        data.insert(data.cend(), tmp.cbegin(), tmp.cend());
                }
                constexpr int GRID_SIZE = SAMPLE_COUNT;
                write_to_file(sampler_name(sampler), sampler_file_name(sampler), GRID_SIZE, data);
        }
        {
                HaltonSampler<N, T> sampler;
                std::vector<Vector<N, T>> data(PASS_COUNT * SAMPLE_COUNT);
                for (Vector<N, T>& v : data)
                {
                        v = sampler.generate();
                }
                constexpr int GRID_SIZE = ONE_DIMENSION_SAMPLE_COUNT;
                write_to_file(sampler_name(sampler), sampler_file_name(sampler), GRID_SIZE, data);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void write_to_files()
{
        write_to_files<N, T, RandomEngine>(false);
        write_to_files<N, T, RandomEngine>(true);
}

template <typename T, typename RandomEngine>
void write_to_files()
{
        write_to_files<2, T, RandomEngine>();
        write_to_files<3, T, RandomEngine>();
}

void write_to_files()
{
        write_to_files<float, std::mt19937_64>();
        LOG("");
        write_to_files<double, std::mt19937_64>();
        LOG("");
        write_to_files<long double, std::mt19937_64>();
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance(bool shuffle)
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int ITER_COUNT = 1'000'000;
        constexpr int SAMPLE_COUNT = power<N>(one_dimension_sample_count<N>());

        std::vector<Vector<N, T>> data;

        LOG(std::string("Testing performance, ") + (shuffle ? "shuffle, " : "") + to_string(N) + "D");

        {
                StratifiedJitteredSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                TimePoint start_time = time();
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        sampler.generate(random_engine, &data);
                }
                LOG(sampler_name(sampler) + ": time = " + to_string_fixed(duration_from(start_time), 5)
                    + " seconds, size = " + to_string(data.size()));
        }
        {
                LatinHypercubeSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                TimePoint start_time = time();
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        sampler.generate(random_engine, &data);
                }
                LOG(sampler_name(sampler) + ": time = " + to_string_fixed(duration_from(start_time), 5)
                    + " seconds, size = " + to_string(data.size()));
        }
        {
                HaltonSampler<N, T> sampler;
                TimePoint start_time = time();
                data.resize(SAMPLE_COUNT);
                for (int i = 0; i < ITER_COUNT; ++i)
                {
                        for (int j = 0; j < SAMPLE_COUNT; ++j)
                        {
                                data[j] = sampler.generate();
                        }
                }
                LOG(sampler_name(sampler) + ": time = " + to_string_fixed(duration_from(start_time), 5)
                    + " seconds, size = " + to_string(data.size()));
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        test_performance<N, T, RandomEngine>(false);
        test_performance<N, T, RandomEngine>(true);
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

template <typename T>
void test_performance()
{
        test_performance<T, std::mt19937>();
        LOG("");
        test_performance<T, std::mt19937_64>();
}

void test_sampler_performance()
{
        test_performance<float>();
        LOG("");
        test_performance<double>();
        LOG("");
        test_performance<long double>();
}

template <typename T, typename RandomEngine>
std::array<T, 2> min_max_for_sampler(RandomEngine& random_engine)
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

template <std::size_t N, typename T, typename RandomEngine>
T test_discrepancy(
        const std::string& sampler_name,
        T min,
        T max,
        const std::vector<Vector<N, T>>& data,
        T discrepancy_limit,
        RandomEngine& random_engine)
{
        LOG(sampler_name + ", " + to_string(N) + "d, " + type_name<T>() + ", [" + to_string(min) + ", " + to_string(max)
            + ")");

        constexpr int BOX_COUNT = 10'000;

        T discrepancy = compute_discrepancy(min, max, data, BOX_COUNT, random_engine);
        LOG("discrepancy = " + to_string(discrepancy));

        if (!(discrepancy < discrepancy_limit))
        {
                error(sampler_name + " discrepancy " + to_string(discrepancy) + " is out of discrepancy limit "
                      + to_string(discrepancy_limit));
        }

        return discrepancy;
}

template <std::size_t N, typename T>
T test_discrepancy_stratified_jittered_type(int sample_count, std::type_identity_t<T> max_discrepancy)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const auto [min, max] = min_max_for_sampler<T>(engine);

        StratifiedJitteredSampler<N, T> sampler(min, max, sample_count, true);
        std::vector<Vector<N, T>> data;
        sampler.generate(engine, &data);
        return test_discrepancy(sampler_name(sampler), min, max, data, max_discrepancy, engine);
}

template <std::size_t N, typename T>
T test_discrepancy_latin_hypercube_type(int sample_count, std::type_identity_t<T> max_discrepancy)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        const auto [min, max] = min_max_for_sampler<T>(engine);

        LatinHypercubeSampler<N, T> sampler(min, max, sample_count, true);
        std::vector<Vector<N, T>> data;
        sampler.generate(engine, &data);
        return test_discrepancy(sampler_name(sampler), min, max, data, max_discrepancy, engine);
}

template <std::size_t N, typename T>
T test_discrepancy_halton_type(int sample_count, std::type_identity_t<T> max_discrepancy)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        HaltonSampler<N, T> sampler;
        std::vector<Vector<N, T>> data(sample_count);
        for (Vector<N, T>& v : data)
        {
                v = sampler.generate();
        }
        return test_discrepancy(sampler_name(sampler), T(0), T(1), data, max_discrepancy, engine);
}

template <std::size_t N>
double test_discrepancy_stratified_jittered(int sample_count, double max_discrepancy)
{
        double f = test_discrepancy_stratified_jittered_type<N, float>(sample_count, max_discrepancy);
        double d = test_discrepancy_stratified_jittered_type<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

template <std::size_t N>
double test_discrepancy_latin_hypercube(int sample_count, double max_discrepancy)
{
        double f = test_discrepancy_latin_hypercube_type<N, float>(sample_count, max_discrepancy);
        double d = test_discrepancy_latin_hypercube_type<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

template <std::size_t N>
double test_discrepancy_halton(int sample_count, double max_discrepancy)
{
        double f = test_discrepancy_halton_type<N, float>(sample_count, max_discrepancy);
        double d = test_discrepancy_halton_type<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

void test_sampler_discrepancy()
{
        LOG("Test sampler discrepancy");

        write_to_files();

        LOG("");

        {
                constexpr unsigned N = 2;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_discrepancy_stratified_jittered<N>(SAMPLE_COUNT, 0.135);
                test_discrepancy_latin_hypercube<N>(SAMPLE_COUNT, 0.135);
                test_discrepancy_halton<N>(SAMPLE_COUNT, 0.06);
        }
        {
                constexpr unsigned N = 3;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_discrepancy_stratified_jittered<N>(SAMPLE_COUNT, 0.042);
                test_discrepancy_latin_hypercube<N>(SAMPLE_COUNT, 0.046);
                test_discrepancy_halton<N>(SAMPLE_COUNT, 0.015);
        }
        {
                constexpr unsigned N = 4;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_discrepancy_stratified_jittered<N>(SAMPLE_COUNT, 0.012);
                test_discrepancy_latin_hypercube<N>(SAMPLE_COUNT, 0.012);
                test_discrepancy_halton<N>(SAMPLE_COUNT, 0.003);
        }
}

TEST_SMALL("Sampler discrepancy", test_sampler_discrepancy)
TEST_PERFORMANCE("Samplers", test_sampler_performance)
}
}
