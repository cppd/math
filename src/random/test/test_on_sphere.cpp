/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "test_on_sphere.h"

#include "../sphere.h"

#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/name.h>

#include <fstream>
#include <random>
#include <string>

namespace ns::random
{
namespace impl = sphere_implementation;

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
std::filesystem::path samples_file_name(const std::string_view& name)
{
        return path_from_utf8(
                "samples_on_sphere_" + replace_space(name) + "_" + to_string(N) + "d_" + replace_space(type_name<T>())
                + ".txt");
}

template <std::size_t N, typename T, typename Generator>
void write_samples_to_file(const std::string_view& name, int count, const Generator& g)
{
        std::ofstream file(std::filesystem::temp_directory_path() / samples_file_name<N, T>(name));

        for (int i = 0; i < count; ++i)
        {
                file << to_string(g()) << "\n";
        }
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance_rejection(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;

        TimePoint start_time = time();

        for (int i = 0; i < count; ++i)
        {
                v = impl::random_on_sphere_by_rejection<N, T>(random_engine);
        }

        LOG("Rejection: time = " + to_string_fixed(duration_from(start_time), 5)
            + " seconds, count = " + to_string(count));
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance_normal_distribution(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;

        TimePoint start_time = time();

        for (int i = 0; i < count; ++i)
        {
                v = impl::random_on_sphere_by_normal_distribution<N, T>(random_engine);
        }

        LOG("Normal distribution: time = " + to_string_fixed(duration_from(start_time), 5)
            + " seconds, count = " + to_string(count));
}

template <std::size_t N, typename T, typename RandomEngine>
void write_samples_to_files()
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int count = N == 2 ? 200 : 1e4;

        LOG("Writing samples " + to_string(N) + "D");

        write_samples_to_file<N, T>(
                "rejection", count,
                [&]()
                {
                        return impl::random_on_sphere_by_rejection<N, T>(random_engine);
                });

        write_samples_to_file<N, T>(
                "normal distribution", count,
                [&]()
                {
                        return impl::random_on_sphere_by_normal_distribution<N, T>(random_engine);
                });
}

template <std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        RandomEngine random_engine = create_engine<RandomEngine>();

        constexpr int count = 5e6;

        LOG("Testing performance " + to_string(N) + "D");
        test_performance_rejection<N, T>(count, random_engine);
        test_performance_normal_distribution<N, T>(count, random_engine);
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
        test_performance<7, T, RandomEngine>();
        test_performance<8, T, RandomEngine>();
        test_performance<9, T, RandomEngine>();
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
}

void test_on_sphere()
{
        write_samples_to_files<std::mt19937_64>();

        LOG("");
        test_performance<float>();
        LOG("");
        test_performance<double>();
        LOG("");
        test_performance<long double>();
}
}
