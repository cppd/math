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

#include "test.h"

#include <src/com/error.h>
#include <src/com/exception.h>
#include <src/com/random/pcg.h>
#include <src/com/variant.h>

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
        const char* const type_name,
        ProgressRatios* const progress_ratios)
{
        const std::string name = [&]()
        {
                std::ostringstream oss;
                oss << "Self-Test, " << type_name << ", " << test_name;
                return oss.str();
        }();

        ProgressRatio progress(progress_ratios, name);

        const auto visitors = Visitors{
                [&progress](void (*const f)())
                {
                        progress.set(0);
                        f();
                },
                [&progress](void (*const f)(ProgressRatio*))
                {
                        f(&progress);
                }};

        catch_all(
                name,
                [&]()
                {
                        std::visit(visitors, test);
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

void Tests::run_small(const std::string_view& name, ProgressRatios* const progress_ratios) const
{
        const auto iter = small_tests_.find(name);
        if (iter == small_tests_.cend())
        {
                std::ostringstream oss;
                oss << SMALL << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, SMALL, progress_ratios);
}

void Tests::run_large(const std::string_view& name, ProgressRatios* const progress_ratios) const
{
        const auto iter = large_tests_.find(name);
        if (iter == large_tests_.cend())
        {
                std::ostringstream oss;
                oss << LARGE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, LARGE, progress_ratios);
}

void Tests::run_performance(const std::string_view& name, ProgressRatios* const progress_ratios) const
{
        const auto iter = performance_tests_.find(name);
        if (iter == performance_tests_.cend())
        {
                std::ostringstream oss;
                oss << PERFORMANCE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, iter->first, PERFORMANCE, progress_ratios);
}

void Tests::run_small(ProgressRatios* const progress_ratios) const
{
        run_small(small_names(), progress_ratios);
}

void Tests::run_large(ProgressRatios* const progress_ratios) const
{
        run_large(large_names(), progress_ratios);
}

void Tests::run_performance(ProgressRatios* const progress_ratios) const
{
        run_performance(performance_names(), progress_ratios);
}

void Tests::run_small(std::vector<std::string> names, ProgressRatios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_small(name, progress_ratios);
        }
}

void Tests::run_large(std::vector<std::string> names, ProgressRatios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_large(name, progress_ratios);
        }
}

void Tests::run_performance(std::vector<std::string> names, ProgressRatios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_performance(name, progress_ratios);
        }
}
}
