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

#include <src/com/chrono.h>
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
double test_performance_on_sphere_by_rejection(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;

        Clock::time_point start_time = Clock::now();

        for (int i = 0; i < count; ++i)
        {
                v = impl::uniform_on_sphere_by_rejection<N, T>(random_engine);
        }

        return duration_from(start_time);
}

template <std::size_t N, typename T, typename RandomEngine>
double test_performance_on_sphere_by_normal_distribution(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;

        Clock::time_point start_time = Clock::now();

        for (int i = 0; i < count; ++i)
        {
                v = impl::uniform_on_sphere_by_normal_distribution<N, T>(random_engine);
        }

        return duration_from(start_time);
}

template <std::size_t N, typename T, typename RandomEngine>
double test_performance_in_sphere_by_rejection(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;
        static T v_length_square;

        Clock::time_point start_time = Clock::now();

        for (int i = 0; i < count; ++i)
        {
                impl::uniform_in_sphere_by_rejection(random_engine, v, v_length_square);
        }

        return duration_from(start_time);
}

template <std::size_t N, typename T, typename RandomEngine>
double test_performance_in_sphere_by_normal_distribution(int count, RandomEngine& random_engine)
{
        static Vector<N, T> v;
        static T v_length_square;

        Clock::time_point start_time = Clock::now();

        for (int i = 0; i < count; ++i)
        {
                impl::uniform_in_sphere_by_normal_distribution(random_engine, v, v_length_square);
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
        error_fatal("Unknown type " + to_string(static_cast<long long>(type)));
}

std::string time_to_string(double v)
{
        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(5);
        oss << std::setw(8);
        oss << v;
        return oss.str();
}

template <SampleType SAMPLE_TYPE, std::size_t N, typename T, typename RandomEngine>
void write_description()
{
        std::ostringstream oss;
        oss << type_to_string(SAMPLE_TYPE) << ", " << N << "D, " << type_name<T>() << ", " << ENGINE_NAME<RandomEngine>;
        LOG(oss.str());
}

template <SampleType SAMPLE_TYPE, std::size_t N, typename T, typename RandomEngine>
void test_performance()
{
        constexpr int COUNT = 5'000'000;

        write_description<SAMPLE_TYPE, N, T, RandomEngine>();

        RandomEngine random_engine = create_engine<RandomEngine>();

        double t;
        switch (SAMPLE_TYPE)
        {
        case SampleType::ON_SPHERE:
        {
                t = test_performance_on_sphere_by_rejection<N, T>(COUNT, random_engine);
                LOG("  Rejection: " + time_to_string(t));
                t = test_performance_on_sphere_by_normal_distribution<N, T>(COUNT, random_engine);
                LOG("  Normal   : " + time_to_string(t));
                return;
        }
        case SampleType::IN_SPHERE:
        {
                t = test_performance_in_sphere_by_rejection<N, T>(COUNT, random_engine);
                LOG("  Rejection: " + time_to_string(t));
                t = test_performance_in_sphere_by_normal_distribution<N, T>(COUNT, random_engine);
                LOG("  Normal   : " + time_to_string(t));
                return;
        }
        }
        error_fatal("Unknown type " + to_string(static_cast<long long>(SAMPLE_TYPE)));
}

template <SampleType SAMPLE_TYPE, std::size_t N, typename T>
void test_performance()
{
        test_performance<SAMPLE_TYPE, N, T, std::mt19937>();
        test_performance<SAMPLE_TYPE, N, T, std::mt19937_64>();
}

template <SampleType SAMPLE_TYPE, typename T>
void test_performance()
{
        static_assert(std::is_floating_point_v<T>);

        test_performance<SAMPLE_TYPE, 2, T>();
        LOG("");
        test_performance<SAMPLE_TYPE, 3, T>();
        LOG("");
        test_performance<SAMPLE_TYPE, 4, T>();
        LOG("");
        test_performance<SAMPLE_TYPE, 5, T>();
        LOG("");
        test_performance<SAMPLE_TYPE, 6, T>();
        LOG("");
        test_performance<SAMPLE_TYPE, 7, T>();
}

template <SampleType SAMPLE_TYPE>
void test_performance()
{
        test_performance<SAMPLE_TYPE, float>();
        LOG("");
        test_performance<SAMPLE_TYPE, double>();
}

void test()
{
        test_performance<SampleType::ON_SPHERE>();
        LOG("");
        test_performance<SampleType::IN_SPHERE>();
}

TEST_PERFORMANCE("Uniform Sphere Samples", test)
}
}
