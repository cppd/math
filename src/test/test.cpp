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

#include <src/com/exception.h>

namespace ns::test
{
Tests& Tests::instance()
{
        static Tests tests;
        return tests;
}

void Tests::add(const char* name, std::function<void(ProgressRatio* progress)> function)
{
        m_tests.emplace_back(name, std::move(function));
}

void Tests::run(ProgressRatios* progress_ratios) const
{
        const std::string s = "Self-Test, ";

        for (const Test& test : m_tests)
        {
                catch_all(
                        s + test.name,
                        [&]()
                        {
                                ProgressRatio progress(progress_ratios, s + test.name);
                                test.function(&progress);
                        });
        }
}
}
