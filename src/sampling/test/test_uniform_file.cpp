/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <array>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace ns::sampling::test
{
namespace
{
namespace impl = sphere_implementation;

std::string replace_space(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return res;
}

template <std::size_t N, typename T>
std::string samples_file_name(const std::string_view name)
{
        std::ostringstream oss;
        oss << "samples_" << replace_space(name) << "_" << N << "d_" << replace_space(type_name<T>()) << ".txt";
        return oss.str();
}

template <std::size_t N, typename T>
std::filesystem::path samples_file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(samples_file_name<N, T>(name));
}

template <std::size_t N, typename T, typename Generator>
void write_samples_to_file(const std::string_view name, const int count, const Generator& g)
{
        std::ofstream file(samples_file_path<N, T>(name));
        for (int i = 0; i < count; ++i)
        {
                file << to_string(g()) << "\n";
        }
}

template <std::size_t N, typename T>
void write_samples_to_files()
{
        constexpr int COUNT = (N == 2) ? 200 : 10'000;

        LOG("Writing samples <" + to_string(N) + ", " + type_name<T>() + ">");

        PCG engine;

        write_samples_to_file<N, T>(
                "on sphere rejection", COUNT,
                [&]()
                {
                        return impl::uniform_on_sphere_by_rejection<N, T>(engine);
                });

        write_samples_to_file<N, T>(
                "on sphere normal distribution", COUNT,
                [&]()
                {
                        return impl::uniform_on_sphere_by_normal_distribution<N, T>(engine);
                });

        write_samples_to_file<N, T>(
                "in sphere rejection", COUNT,
                [&]()
                {
                        Vector<N, T> v;
                        T v_length_square;
                        impl::uniform_in_sphere_by_rejection(engine, v, v_length_square);
                        return v;
                });

        write_samples_to_file<N, T>(
                "in sphere normal distribution", COUNT,
                [&]()
                {
                        Vector<N, T> v;
                        T v_length_square;
                        impl::uniform_in_sphere_by_normal_distribution(engine, v, v_length_square);
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
                                res[N] = Vector<N, T>(1 / std::sqrt(T{N}));
                                return res;
                        }();
                        return uniform_in_simplex(engine, vertices);
                });

        constexpr std::array<Vector<N, T>, N> VECTORS = []
        {
                std::array<Vector<N, T>, N> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = Vector<N, T>(0);
                        res[i][i] = 2;
                }
                return res;
        }();

        write_samples_to_file<N, T>(
                "in parallelotope", COUNT,
                [&]()
                {
                        return uniform_in_parallelotope(engine, VECTORS);
                });

        std::vector<Vector<N, T>> samples;
        StratifiedJitteredSampler<N, T>(0, 1, COUNT, false).generate(engine, &samples);
        std::size_t sample = 0;
        write_samples_to_file<N, T>(
                "in parallelotope with sampler", samples.size(),
                [&]()
                {
                        ASSERT(sample < samples.size());
                        return uniform_in_parallelotope(VECTORS, samples[sample++]);
                });
}

template <typename T>
void write_samples_to_files()
{
        write_samples_to_files<2, T>();
        write_samples_to_files<3, T>();
        write_samples_to_files<4, T>();
}

void test()
{
        write_samples_to_files<float>();
        write_samples_to_files<double>();
        write_samples_to_files<long double>();
}

TEST_SMALL("Uniform Samples File", test)
}
}
