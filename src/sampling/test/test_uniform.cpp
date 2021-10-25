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

#include "../sphere_uniform.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>
#include <sstream>
#include <string>

namespace ns::sampling::test
{
namespace
{
namespace impl = sphere_implementation;

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

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> uniform_in_sphere_by_rejection(RandomEngine& random_engine)
{
        Vector<N, T> v;
        T v_length_square;
        impl::uniform_in_sphere_by_rejection(random_engine, v, v_length_square);
        return v;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> uniform_in_sphere_by_normal_distribution(RandomEngine& random_engine)
{
        Vector<N, T> v;
        T v_length_square;
        impl::uniform_in_sphere_by_normal_distribution(random_engine, v, v_length_square);
        return v;
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_on_sphere_by_rejection(RandomEngine& random_engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(impl::uniform_on_sphere_by_rejection<N, T>(random_engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_on_sphere_by_normal_distribution(RandomEngine& random_engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(impl::uniform_on_sphere_by_normal_distribution<N, T>(random_engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_in_sphere_by_rejection(RandomEngine& random_engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(uniform_in_sphere_by_rejection<N, T>(random_engine));
        }
        return duration_from(start_time);
}

template <int COUNT, std::size_t N, typename T, typename RandomEngine>
double test_in_sphere_by_normal_distribution(RandomEngine& random_engine)
{
        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                do_not_optimize(uniform_in_sphere_by_normal_distribution<N, T>(random_engine));
        }
        return duration_from(start_time);
}

enum class SampleType
{
        ON_SPHERE,
        IN_SPHERE
};

std::string type_to_string(SampleType type)
{
        switch (type)
        {
        case SampleType::ON_SPHERE:
        {
                return "On Sphere";
        }
        case SampleType::IN_SPHERE:
        {
                return "In Sphere";
        }
        }
        error_fatal("Unknown type " + to_string(enum_to_int(type)));
}

template <SampleType SAMPLE_TYPE, std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        constexpr int COUNT = 5'000'000;

        std::ostringstream oss;

        oss << std::fixed;
        oss << std::setprecision(5);

        oss << type_to_string(SAMPLE_TYPE);
        oss << " <" << N << ", " << type_name<T>() << ", " << ENGINE_NAME<RandomEngine> << ">:";

        RandomEngine random_engine = create_engine<RandomEngine>();

        switch (SAMPLE_TYPE)
        {
        case SampleType::ON_SPHERE:
        {
                const double r = test_on_sphere_by_rejection<COUNT, N, T>(random_engine);
                oss << " rejection " << r;
                const double n = test_on_sphere_by_normal_distribution<COUNT, N, T>(random_engine);
                oss << ", normal " << n;
                LOG(oss.str());
                return;
        }
        case SampleType::IN_SPHERE:
        {
                const double r = test_in_sphere_by_rejection<COUNT, N, T>(random_engine);
                oss << " rejection " << r;
                const double n = test_in_sphere_by_normal_distribution<COUNT, N, T>(random_engine);
                oss << ", normal " << n;
                LOG(oss.str());
                return;
        }
        }
        error_fatal("Unknown type " + to_string(enum_to_int(SAMPLE_TYPE)));
}

template <SampleType SAMPLE_TYPE, typename T, typename RandomEngine>
void test_performance()
{
        static_assert(std::is_floating_point_v<T>);

        test_performance<SAMPLE_TYPE, 2, T, RandomEngine>();
        test_performance<SAMPLE_TYPE, 3, T, RandomEngine>();
        test_performance<SAMPLE_TYPE, 4, T, RandomEngine>();
        test_performance<SAMPLE_TYPE, 5, T, RandomEngine>();
        test_performance<SAMPLE_TYPE, 6, T, RandomEngine>();
        test_performance<SAMPLE_TYPE, 7, T, RandomEngine>();
}

template <SampleType SAMPLE_TYPE>
void test_performance()
{
        test_performance<SAMPLE_TYPE, float, std::mt19937>();
        test_performance<SAMPLE_TYPE, float, std::mt19937_64>();
        test_performance<SAMPLE_TYPE, double, std::mt19937>();
        test_performance<SAMPLE_TYPE, double, std::mt19937_64>();
}

void test()
{
        test_performance<SampleType::ON_SPHERE>();
        test_performance<SampleType::IN_SPHERE>();
}

TEST_PERFORMANCE("Uniform sphere samples", test)
}
}
