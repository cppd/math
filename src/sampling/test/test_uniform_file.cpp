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

#include "../parallelotope_uniform.h"
#include "../simplex_uniform.h"
#include "../sj_sampler.h"
#include "../sphere_uniform.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <fstream>
#include <random>
#include <sstream>
#include <string>

namespace ns::sampling::test
{
namespace
{
namespace impl = sphere_implementation;

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
constexpr std::string_view ENGINE_NAME = []
{
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::mt19937>)
        {
                return "std::mt19937";
        }
        if constexpr (std::is_same_v<std::remove_cv_t<T>, std::mt19937_64>)
        {
                return "std::mt19937_64";
        }
}();

template <std::size_t N, typename T>
std::string samples_file_name(const std::string_view& name)
{
        std::ostringstream oss;
        oss << "samples_" << replace_space(name) << "_" << N << "d_" << replace_space(type_name<T>()) << ".txt";
        return oss.str();
}

template <std::size_t N, typename T, typename Generator>
void write_samples_to_file(const std::string_view& name, int count, const Generator& g)
{
        std::ofstream file(std::filesystem::temp_directory_path() / path_from_utf8(samples_file_name<N, T>(name)));

        for (int i = 0; i < count; ++i)
        {
                file << to_string(g()) << "\n";
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void write_samples_to_files()
{
        constexpr int COUNT = (N == 2) ? 200 : 10'000;

        RandomEngine random_engine = create_engine<RandomEngine>();

        LOG("Writing samples " + to_string(N) + "D");

        write_samples_to_file<N, T>(
                "on sphere rejection", COUNT,
                [&]()
                {
                        return impl::uniform_on_sphere_by_rejection<N, T>(random_engine);
                });

        write_samples_to_file<N, T>(
                "on sphere normal distribution", COUNT,
                [&]()
                {
                        return impl::uniform_on_sphere_by_normal_distribution<N, T>(random_engine);
                });

        write_samples_to_file<N, T>(
                "in sphere rejection", COUNT,
                [&]()
                {
                        Vector<N, T> v;
                        T v_length_square;
                        impl::uniform_in_sphere_by_rejection(random_engine, v, v_length_square);
                        return v;
                });

        write_samples_to_file<N, T>(
                "in sphere normal distribution", COUNT,
                [&]()
                {
                        Vector<N, T> v;
                        T v_length_square;
                        impl::uniform_in_sphere_by_normal_distribution(random_engine, v, v_length_square);
                        return v;
                });

        write_samples_to_file<N, T>(
                "in simplex", COUNT,
                [&]()
                {
                        static const std::array<Vector<N, T>, N + 1> vertices = []
                        {
                                std::array<Vector<N, T>, N + 1> res;
                                for (std::size_t i = 0; i < N; ++i)
                                {
                                        res[i] = Vector<N, T>(0);
                                        res[i][i] = 1;
                                }
                                res[N] = Vector<N, T>(1 / std::sqrt(T(N)));
                                return res;
                        }();
                        return uniform_in_simplex(vertices, random_engine);
                });

        std::vector<Vector<N, T>> samples;
        StratifiedJitteredSampler<N, T>(0, 1, COUNT, false).generate(random_engine, &samples);
        std::size_t sample = 0;
        write_samples_to_file<N, T>(
                "in parallelotope", samples.size(),
                [&]()
                {
                        static constexpr std::array<Vector<N, T>, N> VECTORS = []
                        {
                                std::array<Vector<N, T>, N> res;
                                for (std::size_t i = 0; i < N; ++i)
                                {
                                        res[i] = Vector<N, T>(0);
                                        res[i][i] = 2;
                                }
                                return res;
                        }();
                        ASSERT(sample < samples.size());
                        return uniform_in_parallelotope(VECTORS, samples[sample++]);
                });
}

template <typename T, typename RandomEngine>
void write_description()
{
        std::ostringstream oss;
        oss << "Files <" << type_name<T>() << ", " << ENGINE_NAME<RandomEngine> << ">";
        LOG(oss.str());
}

template <typename T, typename RandomEngine>
void write_samples_to_files()
{
        static_assert(std::is_floating_point_v<T>);

        write_description<T, RandomEngine>();

        write_samples_to_files<2, T, RandomEngine>();
        write_samples_to_files<3, T, RandomEngine>();
        write_samples_to_files<4, T, RandomEngine>();
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

void test()
{
        write_samples_to_files<std::mt19937_64>();
}

TEST_PERFORMANCE("Uniform Samples File", test)
}
}
