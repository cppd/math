/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/error.h>
#include <src/progress/progress.h>

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ns::test
{
class Tests final
{
        friend struct AddSmallTest;
        friend struct AddLargeTest;
        friend struct AddPerformanceTest;

        using Test = std::variant<void (*)(), void (*)(progress::Ratio*)>;

        static void run(
                const Test& test,
                const std::string_view& test_name,
                const char* type_name,
                progress::Ratios* progress_ratios);

        std::unordered_map<std::string_view, Test> small_tests_;
        std::unordered_map<std::string_view, Test> large_tests_;
        std::unordered_map<std::string_view, Test> performance_tests_;

        Tests() = default;

        template <typename T>
        void add_small(const char* const name, T* const function)
        {
                small_tests_.emplace(name, function);
        }

        template <typename T>
        void add_large(const char* const name, T* const function)
        {
                large_tests_.emplace(name, function);
        }

        template <typename T>
        void add_performance(const char* const name, T* const function)
        {
                performance_tests_.emplace(name, function);
        }

public:
        static Tests& instance();

        [[nodiscard]] std::vector<std::string> small_names() const;
        [[nodiscard]] std::vector<std::string> large_names() const;
        [[nodiscard]] std::vector<std::string> performance_names() const;

        void run_small(const std::string_view& name, progress::Ratios* progress_ratios) const;
        void run_large(const std::string_view& name, progress::Ratios* progress_ratios) const;
        void run_performance(const std::string_view& name, progress::Ratios* progress_ratios) const;

        void run_small(progress::Ratios* progress_ratios) const;
        void run_large(progress::Ratios* progress_ratios) const;
        void run_performance(progress::Ratios* progress_ratios) const;

        void run_small(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
        void run_large(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
        void run_performance(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
};

struct AddSmallTest final
{
        template <typename T>
        AddSmallTest(const char* const name, T* const function) noexcept
        {
                try
                {
                        Tests::instance().add_small(name, function);
                }
                catch (...)
                {
                        error_fatal("Error adding small test");
                }
        }
};

struct AddLargeTest final
{
        template <typename T>
        AddLargeTest(const char* const name, T* const function) noexcept
        {
                try
                {
                        Tests::instance().add_large(name, function);
                }
                catch (...)
                {
                        error_fatal("Error adding large test");
                }
        }
};

struct AddPerformanceTest final
{
        template <typename T>
        AddPerformanceTest(const char* const name, T* const function) noexcept
        {
                try
                {
                        Tests::instance().add_performance(name, function);
                }
                catch (...)
                {
                        error_fatal("Error adding performance test");
                }
        }
};

#if defined(__clang__)
#define TEST_IMPL_PRAGMA_PUSH _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"")
#define TEST_IMPL_PRAGMA_POP _Pragma("GCC diagnostic pop")
#else
#define TEST_IMPL_PRAGMA_PUSH
#define TEST_IMPL_PRAGMA_POP
#endif

#define TEST_IMPL_UNIQUE_NAME_2(n) test_name_##n
#define TEST_IMPL_UNIQUE_NAME(n) TEST_IMPL_UNIQUE_NAME_2(n)

#define TEST_IMPL_ADD(type, name, f)                                         \
        namespace                                                            \
        {                                                                    \
        TEST_IMPL_PRAGMA_PUSH                                                \
        const ::ns::test::type TEST_IMPL_UNIQUE_NAME(__LINE__){(name), (f)}; \
        TEST_IMPL_PRAGMA_POP                                                 \
        }

#define TEST_SMALL(name, f) TEST_IMPL_ADD(AddSmallTest, (name), (f))
#define TEST_LARGE(name, f) TEST_IMPL_ADD(AddLargeTest, (name), (f))
#define TEST_PERFORMANCE(name, f) TEST_IMPL_ADD(AddPerformanceTest, (name), (f))
}
