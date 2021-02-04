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

#pragma once

#include <src/progress/progress.h>

#include <string_view>
#include <variant>
#include <vector>

namespace ns::test
{
class Tests final
{
        friend struct AddSmallTest;
        friend struct AddLargeTest;
        friend struct AddPerformanceTest;

        struct Test final
        {
                const char* name;
                std::variant<void (*)(), void (*)(ProgressRatio*)> function;
                template <typename T>
                Test(const char* name, T* function) : name(name), function(function)
                {
                        static_assert(std::is_function_v<T>);
                }
        };

        static void run(const Test& test, const char* type, ProgressRatios* progress_ratios);
        static void run(std::vector<Test> tests, const char* type, ProgressRatios* progress_ratios);

        std::vector<Test> m_small_tests;
        std::vector<Test> m_large_tests;
        std::vector<Test> m_performance_tests;

        Tests() = default;

        template <typename T>
        void add_small(const char* name, T* function)
        {
                m_small_tests.emplace_back(name, function);
        }

        template <typename T>
        void add_large(const char* name, T* function)
        {
                m_large_tests.emplace_back(name, function);
        }

        template <typename T>
        void add_performance(const char* name, T* function)
        {
                m_performance_tests.emplace_back(name, function);
        }

public:
        static Tests& instance();

        void run_small(ProgressRatios* progress_ratios) const;
        void run_large(ProgressRatios* progress_ratios) const;
        void run_performance(ProgressRatios* progress_ratios) const;

        void run_small(const std::string_view& name, ProgressRatios* progress_ratios) const;
        void run_large(const std::string_view& name, ProgressRatios* progress_ratios) const;
        void run_performance(const std::string_view& name, ProgressRatios* progress_ratios) const;
};

struct AddSmallTest final
{
        template <typename T>
        AddSmallTest(const char* name, T* function) noexcept
        {
                Tests::instance().add_small(name, function);
        }
};

struct AddLargeTest final
{
        template <typename T>
        AddLargeTest(const char* name, T* function) noexcept
        {
                Tests::instance().add_large(name, function);
        }
};

struct AddPerformanceTest final
{
        template <typename T>
        AddPerformanceTest(const char* name, T* function) noexcept
        {
                Tests::instance().add_performance(name, function);
        }
};

#define TEST_UNIQUE_NAME_2(n) test_name_##n
#define TEST_UNIQUE_NAME(n) TEST_UNIQUE_NAME_2(n)

#if defined(__clang__)

#define TEST_SMALL(name, f)                                                     \
        namespace                                                               \
        {                                                                       \
        _Pragma("GCC diagnostic push");                                         \
        _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"");            \
        const ::ns::test::AddSmallTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        _Pragma("GCC diagnostic pop");                                          \
        }

#define TEST_LARGE(name, f)                                                     \
        namespace                                                               \
        {                                                                       \
        _Pragma("GCC diagnostic push");                                         \
        _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"");            \
        const ::ns::test::AddLargeTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        _Pragma("GCC diagnostic pop");                                          \
        }

#define TEST_PERFORMANCE(name, f)                                                     \
        namespace                                                                     \
        {                                                                             \
        _Pragma("GCC diagnostic push");                                               \
        _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"");                  \
        const ::ns::test::AddPerformanceTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        _Pragma("GCC diagnostic pop");                                                \
        }

#else

#define TEST_SMALL(name, f)                                                     \
        namespace                                                               \
        {                                                                       \
        const ::ns::test::AddSmallTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        }

#define TEST_LARGE(name, f)                                                     \
        namespace                                                               \
        {                                                                       \
        const ::ns::test::AddLargeTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        }

#define TEST_PERFORMANCE(name, f)                                                     \
        namespace                                                                     \
        {                                                                             \
        const ::ns::test::AddPerformanceTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        }

#endif
}
