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
#include <sstream>

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

void Tests::run(
        const Test& test,
        const std::string_view& test_name,
        const char* type_name,
        ProgressRatios* progress_ratios)
{
        const std::string name = [&]()
        {
                std::ostringstream oss;
                oss << "Self-Test, " << type_name << ", " << test_name;
                return oss.str();
        }();
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

std::vector<std::string> Tests::small_names() const
{
        std::vector<std::string> names;
        names.reserve(small_tests_.size());
        for (const auto& [name, test] : small_tests_)
        {
                names.emplace_back(name);
        }
        return names;
}

std::vector<std::string> Tests::large_names() const
{
        std::vector<std::string> names;
        names.reserve(large_tests_.size());
        for (const auto& [name, test] : large_tests_)
        {
                names.emplace_back(name);
        }
        return names;
}

std::vector<std::string> Tests::performance_names() const
{
        std::vector<std::string> names;
        names.reserve(performance_tests_.size());
        for (const auto& [name, test] : performance_tests_)
        {
                names.emplace_back(name);
        }
        return names;
}

void Tests::run_small(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        auto iter = small_tests_.find(name);
        if (iter == small_tests_.cend())
        {
                std::ostringstream oss;
                oss << SMALL << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, SMALL, progress_ratios);
}

void Tests::run_large(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        auto iter = large_tests_.find(name);
        if (iter == large_tests_.cend())
        {
                std::ostringstream oss;
                oss << LARGE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, LARGE, progress_ratios);
}

void Tests::run_performance(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        auto iter = performance_tests_.find(name);
        if (iter == performance_tests_.cend())
        {
                std::ostringstream oss;
                oss << PERFORMANCE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, PERFORMANCE, progress_ratios);
}

void Tests::run_small(ProgressRatios* progress_ratios) const
{
        run_small(small_names(), progress_ratios);
}

void Tests::run_large(ProgressRatios* progress_ratios) const
{
        run_large(large_names(), progress_ratios);
}

void Tests::run_performance(ProgressRatios* progress_ratios) const
{
        run_performance(performance_names(), progress_ratios);
}

void Tests::run_small(std::vector<std::string> names, ProgressRatios* progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), create_engine<std::mt19937>());
        for (const std::string& name : names)
        {
                run_small(name, progress_ratios);
        }
}

void Tests::run_large(std::vector<std::string> names, ProgressRatios* progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), create_engine<std::mt19937>());
        for (const std::string& name : names)
        {
                run_large(name, progress_ratios);
        }
}

void Tests::run_performance(std::vector<std::string> names, ProgressRatios* progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), create_engine<std::mt19937>());
        for (const std::string& name : names)
        {
                run_performance(name, progress_ratios);
        }
}
}
