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

#include "test.h"

#include <src/com/error.h>
#include <src/com/exception.h>
#include <src/com/random/engine.h>
#include <src/com/variant.h>

#include <random>

namespace ns::test
{
namespace
{
constexpr const char* SMALL = "Small";
constexpr const char* LARGE = "Large";
constexpr const char* PERFORMANCE = "Performance";
}

Tests& Tests::instance()
{
        static Tests tests;
        return tests;
}

void Tests::run(const Test& test, const char* type, ProgressRatios* progress_ratios)
{
        const std::string name = std::string("Self-Test, ") + type + ", " + test.name;
        ProgressRatio progress(progress_ratios, name);
        auto f1 = [&](void (*f)())
        {
                progress.set(0);
                f();
        };
        auto f2 = [&](void (*f)(ProgressRatio*))
        {
                f(&progress);
        };
        catch_all(
                name,
                [&]()
                {
                        std::visit(Visitors{f1, f2}, test.function);
                });
}

void Tests::run(std::vector<Test> tests, const char* type, ProgressRatios* progress_ratios)
{
        std::shuffle(tests.begin(), tests.end(), create_engine<std::mt19937>());
        for (const Test& test : tests)
        {
                run(test, type, progress_ratios);
        }
}

void Tests::run_small(ProgressRatios* progress_ratios) const
{
        run(m_small_tests, SMALL, progress_ratios);
}

void Tests::run_large(ProgressRatios* progress_ratios) const
{
        run(m_large_tests, LARGE, progress_ratios);
}

void Tests::run_performance(ProgressRatios* progress_ratios) const
{
        run(m_performance_tests, PERFORMANCE, progress_ratios);
}

void Tests::run_small(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        bool found = false;
        for (const Test& test : m_small_tests)
        {
                if (name == test.name)
                {
                        run(test, SMALL, progress_ratios);
                        found = true;
                        continue;
                }
        }
        if (!found)
        {
                error(std::string(SMALL) + " test not found " + std::string(name));
        }
}

void Tests::run_large(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        bool found = false;
        for (const Test& test : m_large_tests)
        {
                if (name == test.name)
                {
                        run(test, LARGE, progress_ratios);
                        found = true;
                        continue;
                }
        }
        if (!found)
        {
                error(std::string(LARGE) + " test not found " + std::string(name));
        }
}

void Tests::run_performance(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        bool found = false;
        for (const Test& test : m_performance_tests)
        {
                if (name == test.name)
                {
                        run(test, PERFORMANCE, progress_ratios);
                        found = true;
                        continue;
                }
        }
        if (!found)
        {
                error(std::string(PERFORMANCE) + " test not found " + std::string(name));
        }
}
}
