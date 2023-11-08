/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <algorithm>
#include <sstream>

namespace ns::test
{
namespace
{
constexpr std::string_view SMALL = "Small";
constexpr std::string_view LARGE = "Large";
constexpr std::string_view PERFORMANCE = "Performance";

std::string progress_text(const std::string_view test_name, const std::string_view type_name)
{
        std::string res;
        res += "Self-Test, ";
        res += type_name;
        res += ", ";
        res += test_name;
        return res;
}

template <typename T>
void run(const T& test, const std::string& name, progress::Ratios* const progress_ratios)
{
        progress::Ratio progress(progress_ratios, name);

        const auto visitors = Visitors{
                [&progress](void (*const f)())
                {
                        progress.set(0);
                        f();
                },
                [&progress](void (*const f)(progress::Ratio*))
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
}

std::vector<std::string> Tests::small_names() const
{
        std::vector<std::string> res;
        res.reserve(small_tests_.size());
        for (const auto& [name, test] : small_tests_)
        {
                res.emplace_back(name);
        }
        return res;
}

std::vector<std::string> Tests::large_names() const
{
        std::vector<std::string> res;
        res.reserve(large_tests_.size());
        for (const auto& [name, test] : large_tests_)
        {
                res.emplace_back(name);
        }
        return res;
}

std::vector<std::string> Tests::performance_names() const
{
        std::vector<std::string> res;
        res.reserve(performance_tests_.size());
        for (const auto& [name, test] : performance_tests_)
        {
                res.emplace_back(name);
        }
        return res;
}

void Tests::run_small(const std::string& name, progress::Ratios* const progress_ratios) const
{
        const auto iter = small_tests_.find(name);
        if (iter == small_tests_.cend())
        {
                std::ostringstream oss;
                oss << SMALL << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, progress_text(iter->first, SMALL), progress_ratios);
}

void Tests::run_large(const std::string& name, progress::Ratios* const progress_ratios) const
{
        const auto iter = large_tests_.find(name);
        if (iter == large_tests_.cend())
        {
                std::ostringstream oss;
                oss << LARGE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, progress_text(iter->first, LARGE), progress_ratios);
}

void Tests::run_performance(const std::string& name, progress::Ratios* const progress_ratios) const
{
        const auto iter = performance_tests_.find(name);
        if (iter == performance_tests_.cend())
        {
                std::ostringstream oss;
                oss << PERFORMANCE << " test not found " << name;
                error(oss.str());
        }
        run(iter->second, progress_text(iter->first, PERFORMANCE), progress_ratios);
}

void Tests::run_small(progress::Ratios* const progress_ratios) const
{
        run_small(small_names(), progress_ratios);
}

void Tests::run_large(progress::Ratios* const progress_ratios) const
{
        run_large(large_names(), progress_ratios);
}

void Tests::run_performance(progress::Ratios* const progress_ratios) const
{
        run_performance(performance_names(), progress_ratios);
}

void Tests::run_small(std::vector<std::string> names, progress::Ratios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_small(name, progress_ratios);
        }
}

void Tests::run_large(std::vector<std::string> names, progress::Ratios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_large(name, progress_ratios);
        }
}

void Tests::run_performance(std::vector<std::string> names, progress::Ratios* const progress_ratios) const
{
        std::shuffle(names.begin(), names.end(), PCG());
        for (const std::string& name : names)
        {
                run_performance(name, progress_ratios);
        }
}
}
