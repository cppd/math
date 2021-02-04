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

#include "self_test.h"

#include "test.h"

namespace ns::test
{
namespace
{
void self_test_essential(ProgressRatios* progress_ratios)
{
        Tests::instance().run_small(progress_ratios);
}

void self_test_extended(ProgressRatios* progress_ratios)
{
        Tests::instance().run_large(progress_ratios);
}
}

void self_test(SelfTestType test_type, ProgressRatios* progress_ratios)
{
        switch (test_type)
        {
        case SelfTestType::Essential:
                self_test_essential(progress_ratios);
                break;
        case SelfTestType::Extended:
                self_test_essential(progress_ratios);
                self_test_extended(progress_ratios);
                break;
        }
}
}
