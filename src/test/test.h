/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/progress/progress.h>
#include <src/progress/progress_interfaces.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace ns::test
{
enum class Type
{
        SMALL,
        LARGE,
        PERFORMANCE
};

class Tests final
{
        friend struct AddTest;

        using Test = std::variant<void (*)(), void (*)(progress::Ratio*)>;

        std::unordered_map<std::string, Test> small_tests_;
        std::unordered_map<std::string, Test> large_tests_;
        std::unordered_map<std::string, Test> performance_tests_;

        template <typename T, typename Name>
        void add(const Type type, Name&& name, T* const function)
        {
                const bool inserted = [&]
                {
                        switch (type)
                        {
                        case Type::SMALL:
                                return small_tests_.emplace(std::forward<Name>(name), function).second;
                        case Type::LARGE:
                                return large_tests_.emplace(std::forward<Name>(name), function).second;
                        case Type::PERFORMANCE:
                                return performance_tests_.emplace(std::forward<Name>(name), function).second;
                        }
                        error_fatal(
                                "Unknown test type " + std::to_string(enum_to_int(type)) + ", test name "
                                + std::string(std::forward<Name>(name)));
                }();

                if (!inserted)
                {
                        error_fatal("Not unique test name " + std::string(std::forward<Name>(name)));
                }
        }

        Tests() = default;
        ~Tests() = default;

        static Tests& instance_impl()
        {
                static Tests tests;
                return tests;
        }

public:
        static const Tests& instance()
        {
                return instance_impl();
        }

        [[nodiscard]] std::vector<std::string> small_names() const;
        [[nodiscard]] std::vector<std::string> large_names() const;
        [[nodiscard]] std::vector<std::string> performance_names() const;

        void run_small(const std::string& name, progress::Ratios* progress_ratios) const;
        void run_large(const std::string& name, progress::Ratios* progress_ratios) const;
        void run_performance(const std::string& name, progress::Ratios* progress_ratios) const;

        void run_small(progress::Ratios* progress_ratios) const;
        void run_large(progress::Ratios* progress_ratios) const;
        void run_performance(progress::Ratios* progress_ratios) const;

        void run_small(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
        void run_large(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
        void run_performance(std::vector<std::string> names, progress::Ratios* progress_ratios) const;
};

struct AddTest final
{
        AddTest(const AddTest&) = delete;
        AddTest& operator=(const AddTest&) = delete;
        AddTest(AddTest&&) = delete;
        AddTest& operator=(AddTest&&) = delete;

        template <typename T, typename Name>
        AddTest(const Type type, Name&& name, T* const function) noexcept
        {
                try
                {
                        Tests::instance_impl().add(type, std::forward<Name>(name), function);
                }
                catch (...)
                {
                        error_fatal("Error adding test");
                }
        }

        template <typename T>
        explicit AddTest(const T& tests) noexcept
        {
                try
                {
                        for (const auto& test : tests)
                        {
                                Tests::instance_impl().add(std::get<0>(test), std::get<1>(test), std::get<2>(test));
                        }
                }
                catch (...)
                {
                        error_fatal("Error adding tests");
                }
        }
};

#ifdef __clang__
#define TEST_IMPL_PRAGMA_PUSH _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"")
#define TEST_IMPL_PRAGMA_POP _Pragma("GCC diagnostic pop")
#else
#define TEST_IMPL_PRAGMA_PUSH
#define TEST_IMPL_PRAGMA_POP
#endif

#define TEST_IMPL_UNIQUE_NAME_2(n) test_impl_name_##n
#define TEST_IMPL_UNIQUE_NAME(n) TEST_IMPL_UNIQUE_NAME_2(n)

#define TEST(type, name, f)                                                             \
        namespace                                                                       \
        {                                                                               \
        TEST_IMPL_PRAGMA_PUSH                                                           \
        const ::ns::test::AddTest TEST_IMPL_UNIQUE_NAME(__LINE__){(type), (name), (f)}; \
        TEST_IMPL_PRAGMA_POP                                                            \
        }

#define TESTS(tests)                                                        \
        namespace                                                           \
        {                                                                   \
        TEST_IMPL_PRAGMA_PUSH                                               \
        const ::ns::test::AddTest TEST_IMPL_UNIQUE_NAME(__LINE__){(tests)}; \
        TEST_IMPL_PRAGMA_POP                                                \
        }

#define TEST_SMALL(name, f) TEST(::ns::test::Type::SMALL, (name), (f))
#define TEST_LARGE(name, f) TEST(::ns::test::Type::LARGE, (name), (f))
#define TEST_PERFORMANCE(name, f) TEST(::ns::test::Type::PERFORMANCE, (name), (f))
}
