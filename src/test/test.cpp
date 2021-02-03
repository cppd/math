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
Tests& Tests::instance()
{
        static Tests tests;
        return tests;
}

void Tests::run(const Test& test, ProgressRatios* progress_ratios)
{
        std::string name = "Self-Test, ";
        name += test.name;
        auto f1 = [](const std::function<void()>& f)
        {
                f();
        };
        auto f2 = [&](const std::function<void(ProgressRatio*)>& f)
        {
                ProgressRatio progress(progress_ratios, name);
                f(&progress);
        };
        catch_all(
                name,
                [&]()
                {
                        std::visit(Visitors{f1, f2}, test.function);
                });
}

void Tests::run(ProgressRatios* progress_ratios) const
{
        std::vector<Test> tests(m_tests);
        std::shuffle(tests.begin(), tests.end(), create_engine<std::mt19937>());
        for (const Test& test : tests)
        {
                run(test, progress_ratios);
        }
}

void Tests::run(const std::string_view& name, ProgressRatios* progress_ratios) const
{
        for (const Test& test : m_tests)
        {
                if (name == test.name)
                {
                        run(test, progress_ratios);
                        return;
                }
        }
        error("Test not found " + std::string(name));
}
}
