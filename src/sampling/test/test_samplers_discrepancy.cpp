/*
Copyright (C) 2017-2026 Topological Manifold

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
#include "names.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/sampling/halton_sampler.h>
#include <src/sampling/lh_sampler.h>
#include <src/sampling/sj_sampler.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace ns::sampling::test
{
namespace
{
template <typename T, typename RandomEngine>
std::array<T, 2> min_max_for_sampler(RandomEngine& engine)
{
        T min;
        T max;
        if (std::bernoulli_distribution(0.5)(engine))
        {
                min = std::uniform_real_distribution<T>(-10, 10)(engine);
                max = std::uniform_real_distribution<T>(min + 0.1, min + 10)(engine);
        }
        else if (std::bernoulli_distribution(0.5)(engine))
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
        const std::string& name,
        const T& min,
        const T& max,
        const std::vector<numerical::Vector<N, T>>& data,
        const T& discrepancy_limit,
        RandomEngine& engine)
{
        LOG(name);

        constexpr int BOX_COUNT = 10'000;

        const T discrepancy = compute_discrepancy(min, max, data, BOX_COUNT, engine);
        LOG("Discrepancy = " + to_string(discrepancy));

        if (!(discrepancy < discrepancy_limit))
        {
                std::ostringstream oss;
                oss << name << "\n";
                oss << "Discrepancy " << to_string(discrepancy);
                oss << " is greater than " << to_string(discrepancy_limit);
                error(oss.str());
        }

        return discrepancy;
}

template <std::size_t N, typename T>
T test_stratified_jittered(const unsigned sample_count, const std::type_identity_t<T> max_discrepancy)
{
        PCG engine;

        const auto [min, max] = min_max_for_sampler<T>(engine);

        const StratifiedJitteredSampler<N, T> sampler(min, max, sample_count, true);

        std::vector<numerical::Vector<N, T>> data;
        sampler.generate(engine, &data);
        if (data.size() != sample_count)
        {
                std::ostringstream oss;
                oss << "Error sample count " << data.size() << ", expected " << sample_count;
                error(oss.str());
        }

        std::ostringstream oss;
        oss << sampler_name(sampler) << ", " << N << "D, " << type_name<T>();
        oss << ", [" << to_string(min) << ", " << to_string(max) << ")";

        return test_discrepancy(oss.str(), min, max, data, max_discrepancy, engine);
}

template <std::size_t N, typename T>
T test_latin_hypercube(const unsigned sample_count, const std::type_identity_t<T> max_discrepancy)
{
        PCG engine;

        const auto [min, max] = min_max_for_sampler<T>(engine);

        const LatinHypercubeSampler<N, T> sampler(min, max, sample_count, true);

        std::vector<numerical::Vector<N, T>> data;
        sampler.generate(engine, &data);
        if (data.size() != sample_count)
        {
                std::ostringstream oss;
                oss << "Error sample count " << data.size() << ", expected " << sample_count;
                error(oss.str());
        }

        std::ostringstream oss;
        oss << sampler_name(sampler) << ", " << N << "D, " << type_name<T>();
        oss << ", [" << to_string(min) << ", " << to_string(max) << ")";

        return test_discrepancy(oss.str(), min, max, data, max_discrepancy, engine);
}

template <std::size_t N, typename T>
T test_halton(const int sample_count, const std::type_identity_t<T> max_discrepancy)
{
        PCG engine;

        HaltonSampler<N, T> sampler;

        std::vector<numerical::Vector<N, T>> data(sample_count);
        for (numerical::Vector<N, T>& v : data)
        {
                v = sampler.generate();
        }

        std::ostringstream oss;
        oss << sampler_name(sampler) << ", " << N << "D, " << type_name<T>();

        constexpr T MIN = 0;
        constexpr T MAX = 1;
        return test_discrepancy(oss.str(), MIN, MAX, data, max_discrepancy, engine);
}

template <std::size_t N>
double test_stratified_jittered(const int sample_count, const double max_discrepancy)
{
        const double f = test_stratified_jittered<N, float>(sample_count, max_discrepancy);
        const double d = test_stratified_jittered<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

template <std::size_t N>
double test_latin_hypercube(const int sample_count, const double max_discrepancy)
{
        const double f = test_latin_hypercube<N, float>(sample_count, max_discrepancy);
        const double d = test_latin_hypercube<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

template <std::size_t N>
double test_halton(const int sample_count, const double max_discrepancy)
{
        const double f = test_halton<N, float>(sample_count, max_discrepancy);
        const double d = test_halton<N, double>(sample_count, max_discrepancy);
        return std::max(f, d);
}

void test_sampler_discrepancy()
{
        LOG("Test sampler discrepancy");
        {
                constexpr unsigned N = 2;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_stratified_jittered<N>(SAMPLE_COUNT, 0.15);
                test_latin_hypercube<N>(SAMPLE_COUNT, 0.15);
                test_halton<N>(SAMPLE_COUNT, 0.06);
        }
        {
                constexpr unsigned N = 3;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_stratified_jittered<N>(SAMPLE_COUNT, 0.048);
                test_latin_hypercube<N>(SAMPLE_COUNT, 0.048);
                test_halton<N>(SAMPLE_COUNT, 0.016);
        }
        {
                constexpr unsigned N = 4;
                constexpr int SAMPLE_COUNT = power<N>(10);

                test_stratified_jittered<N>(SAMPLE_COUNT, 0.014);
                test_latin_hypercube<N>(SAMPLE_COUNT, 0.014);
                test_halton<N>(SAMPLE_COUNT, 0.0027);
        }
        LOG("Test sampler discrepancy passed");
}

TEST_SMALL("Sampler Discrepancy", test_sampler_discrepancy)
}
}
