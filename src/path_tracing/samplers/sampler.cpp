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

/*
 По книге

 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.

 7.3 Stratified sampling.
*/

#include "sampler.h"

#include "com/error.h"
#include "com/print.h"

#include <type_traits>
#include <utility>

namespace
{
template <typename T, typename RandomEngine, typename Distribution, size_t... I>
Vector<sizeof...(I), T> random_vector(RandomEngine& engine, Distribution& distribution, std::integer_sequence<size_t, I...>)
{
        return Vector<sizeof...(I), T>((static_cast<void>(I), distribution(engine))...);
}
template <size_t N, typename T, typename RandomEngine, typename Distribution>
Vector<N, T> random_vector(RandomEngine& engine, Distribution& distribution)
{
        return random_vector<T>(engine, distribution, std::make_integer_sequence<size_t, N>());
}

// Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
// Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.
// Функция std::shuffle не подходит, так как надо по отдельному измерению.
template <size_t N, typename T, typename RandomEngine>
void shuffle_one_dimension(RandomEngine& random_engine, size_t dimension, std::vector<Vector<N, T>>* v)
{
        ASSERT(dimension < N);

        using Distribution = std::uniform_int_distribution<size_t>;

        Distribution distribution;
        for (size_t i = v->size() - 1; i > 0; --i)
        {
                size_t j = distribution(random_engine, Distribution::param_type(0, i));
                std::swap((*v)[i][dimension], (*v)[j][dimension]);
        }
}

template <typename T, typename RandomEngine>
void stratified_jittered_samples(RandomEngine& random_engine, int one_dimension_sample_count, std::vector<Vector<2, T>>* samples)
{
        static_assert(std::is_floating_point_v<T>);

        std::uniform_real_distribution<T> urd(0, static_cast<T>(1) / one_dimension_sample_count);

        samples->resize(square(one_dimension_sample_count));

        T count_t = one_dimension_sample_count;

        for (int i = 0, sample = 0; i < one_dimension_sample_count; ++i)
        {
                for (int j = 0; j < one_dimension_sample_count; ++j)
                {
                        (*samples)[sample++] = Vector<2, T>(i / count_t, j / count_t) + random_vector<2, T>(random_engine, urd);
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
void latin_hypercube_samples(RandomEngine& random_engine, int sample_count, std::vector<Vector<N, T>>* samples)
{
        static_assert(std::is_floating_point_v<T>);

        std::uniform_real_distribution<T> urd(0, static_cast<T>(1) / sample_count);

        samples->resize(sample_count);

        T count_t = sample_count;

        // Случайные числа по диагонали
        for (int i = 0; i < sample_count; ++i)
        {
                (*samples)[i] = Vector<N, T>(i / count_t) + random_vector<N, T>(random_engine, urd);
        }

        // Достаточно со второго измерения
        for (unsigned i = 1; i < N; ++i)
        {
                shuffle_one_dimension(random_engine, i, samples);
        }
}
}

StratifiedJitteredSampler::StratifiedJitteredSampler(int samples_per_pixel)
        : m_samples_one_dimension(std::ceil(std::sqrt(samples_per_pixel)))
{
        if (samples_per_pixel < 1)
        {
                error("Stratified jittered sample count (" + to_string(samples_per_pixel) + ") is not a positive integer");
        }

        ASSERT(m_samples_one_dimension > 0 && square(m_samples_one_dimension) >= samples_per_pixel);
}

void StratifiedJitteredSampler::generate(std::mt19937_64& random_engine, std::vector<vec2>* samples) const
{
        stratified_jittered_samples(random_engine, m_samples_one_dimension, samples);
}

LatinHypercubeSampler::LatinHypercubeSampler(int samples_per_pixel) : m_samples_per_pixel(samples_per_pixel)
{
        if (samples_per_pixel < 1)
        {
                error("Latin hypercube sample count (" + to_string(samples_per_pixel) + ") is not a positive integer");
        }
}

void LatinHypercubeSampler::generate(std::mt19937_64& random_engine, std::vector<vec2>* samples) const
{
        latin_hypercube_samples(random_engine, m_samples_per_pixel, samples);
}
