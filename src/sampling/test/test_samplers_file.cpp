/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "names.h"

#include "../halton_sampler.h"
#include "../lh_sampler.h"
#include "../sj_sampler.h"

#include <src/com/exponent.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/settings/directory.h>
#include <src/test/test.h>

#include <cctype>
#include <fstream>
#include <sstream>

namespace ns::sampling::test
{
namespace
{
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
std::string sampler_file_name(const StratifiedJitteredSampler<N, T>& sampler)
{
        std::ostringstream oss;
        oss << "sampler_sjs_";
        if (sampler.shuffled())
        {
                oss << "shuffled_";
        }
        oss << N << "d_" << replace_space(type_name<T>()) << ".txt";
        return oss.str();
}

template <std::size_t N, typename T>
std::string sampler_file_name(const LatinHypercubeSampler<N, T>& sampler)
{
        std::ostringstream oss;
        oss << "sampler_lhc_";
        if (sampler.shuffled())
        {
                oss << "shuffled_";
        }
        oss << N << "d_" << replace_space(type_name<T>()) << ".txt";
        return oss.str();
}

template <std::size_t N, typename T>
std::string sampler_file_name(const HaltonSampler<N, T>&)
{
        std::ostringstream oss;
        oss << "sampler_halton_";
        oss << N << "d_" << replace_space(type_name<T>()) << ".txt";
        return oss.str();
}

template <std::size_t N>
constexpr int one_dimension_sample_count()
{
        static_assert(N >= 2);

        switch (N)
        {
        case 2:
        case 3:
                return 5;
        case 4:
                return 4;
        case 5:
        case 6:
                return 3;
        default:
                return 2;
        }
}

template <std::size_t N, typename T>
void write_to_file(
        const std::string_view name,
        const std::string_view file_name,
        const int grid_size,
        const std::vector<Vector<N, T>>& data)
{
        std::ofstream file(settings::test_directory() / path_from_utf8(file_name));

        file << "Name: " << name << "\n";
        file << "Grid: " << grid_size << "\n";

        for (const Vector<N, T>& v : data)
        {
                file << to_string(v) << "\n";
        }
}

template <std::size_t N, typename T>
void write_to_files(const bool shuffle)
{
        static_assert(std::is_floating_point_v<T>);

        PCG engine;

        constexpr int PASS_COUNT = 10;

        constexpr int ONE_DIMENSION_SAMPLE_COUNT = one_dimension_sample_count<N>();
        constexpr int SAMPLE_COUNT = power<N>(ONE_DIMENSION_SAMPLE_COUNT);

        {
                std::ostringstream oss;
                oss << "Writing samples, ";
                if (shuffle)
                {
                        oss << "shuffle, ";
                }
                oss << type_name<T>() << ", " << N << "D";
                LOG(oss.str());
        }

        {
                const StratifiedJitteredSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                std::vector<Vector<N, T>> data;
                std::vector<Vector<N, T>> tmp;
                for (int i = 0; i < PASS_COUNT; ++i)
                {
                        sampler.generate(engine, &tmp);
                        data.insert(data.cend(), tmp.cbegin(), tmp.cend());
                }
                constexpr int GRID_SIZE = ONE_DIMENSION_SAMPLE_COUNT;
                write_to_file(sampler_name(sampler), sampler_file_name(sampler), GRID_SIZE, data);
        }
        {
                const LatinHypercubeSampler<N, T> sampler(0, 1, SAMPLE_COUNT, shuffle);
                std::vector<Vector<N, T>> data;
                std::vector<Vector<N, T>> tmp;
                for (int i = 0; i < PASS_COUNT; ++i)
                {
                        sampler.generate(engine, &tmp);
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

template <std::size_t N, typename T>
void write_to_files()
{
        write_to_files<N, T>(false);
        write_to_files<N, T>(true);
}

template <typename T>
void write_to_files()
{
        write_to_files<2, T>();
        write_to_files<3, T>();
}

void write()
{
        write_to_files<float>();
        write_to_files<double>();
        write_to_files<long double>();
}

TEST_SMALL("Sampler Files", write)
}
}
