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

#include <functional>
#include <string_view>
#include <variant>
#include <vector>

namespace ns::test
{
class Tests final
{
        friend struct AddTest;

        struct Test final
        {
                const char* name;
                std::variant<std::function<void()>, std::function<void(ProgressRatio*)>> function;
                template <typename T>
                Test(const char* name, T&& function) : name(name), function(std::forward<T>(function))
                {
                }
        };

        static void run(const Test& test, ProgressRatios* progress_ratios);

        std::vector<Test> m_tests;

        Tests() = default;

        template <typename T>
        void add(const char* name, T&& function)
        {
                m_tests.emplace_back(name, std::forward<T>(function));
        }

public:
        static Tests& instance();

        void run(ProgressRatios* progress_ratios) const;
        void run(const std::string_view& name, ProgressRatios* progress_ratios) const;
};

struct AddTest final
{
        template <typename T>
        AddTest(const char* name, T&& function) noexcept
        {
                Tests::instance().add(name, std::forward<T>(function));
        }
};

#define TEST_UNIQUE_NAME_2(n) test_##n
#define TEST_UNIQUE_NAME(n) TEST_UNIQUE_NAME_2(n)

#undef TEST
// clang-format off
#if defined(__clang__)
#define TEST(name, f)                                                             \
        _Pragma("GCC diagnostic push")                                            \
        _Pragma("GCC diagnostic ignored \"-Wglobal-constructors\"")               \
        static const ::ns::test::AddTest TEST_UNIQUE_NAME(__LINE__)((name), (f)); \
        _Pragma("GCC diagnostic pop")
#else
#define TEST(name, f) static const ::ns::test::AddTest TEST_UNIQUE_NAME(__LINE__)((name), (f));
#endif
// clang-format on
}
