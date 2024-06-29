/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "testing.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gui/dialogs/test_selection.h>
#include <src/progress/progress_list.h>
#include <src/test/test.h>

#include <functional>
#include <optional>

namespace ns::process
{
std::function<void(progress::RatioList*)> action_self_test(const TestType test_type)
{
        switch (test_type)
        {
        case TestType::SMALL:
        {
                return [=](progress::RatioList* const progress_list)
                {
                        test::Tests::instance().run_small(progress_list);
                };
        }
        case TestType::ALL:
        {
                std::optional<gui::dialogs::TestSelectionParameters> tests =
                        gui::dialogs::TestSelectionParametersDialog::show(
                                "Select tests", test::Tests::instance().large_names());
                if (!tests || tests->test_names.empty())
                {
                        return nullptr;
                }
                return [=](progress::RatioList* const progress_list)
                {
                        test::Tests::instance().run_small(progress_list);
                        test::Tests::instance().run_large(tests->test_names, progress_list);
                };
        }
        case TestType::BENCHMARK:
        {
                std::optional<gui::dialogs::TestSelectionParameters> tests =
                        gui::dialogs::TestSelectionParametersDialog::show(
                                "Select benchmarks", test::Tests::instance().performance_names());
                if (!tests || tests->test_names.empty())
                {
                        return nullptr;
                }
                return [=](progress::RatioList* const progress_list)
                {
                        test::Tests::instance().run_performance(tests->test_names, progress_list);
                };
        }
        }
        error_fatal("Unknown test type " + to_string(enum_to_int(test_type)));
}
}
