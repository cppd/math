/*
Copyright (C) 2017 Topological Manifold

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

#include "dft/test/dft_test.h"
#include "geometry/test/convex_hull_test.h"
#include "geometry/test/reconstruction_test.h"

namespace
{
void self_test_required(IProgressRatioList* progress_ratio_list, std::string* test_name)
{
        {
                *test_name = "Self-Test, DFT in 2D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                progress.set(0);
                dft_test();
        }
        {
                *test_name = "Self-Test, Convex Hull in 2D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                convex_hull_test(2, &progress);
        }
        {
                *test_name = "Self-Test, Convex Hull in 3D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                convex_hull_test(3, &progress);
        }
        {
                *test_name = "Self-Test, Convex Hull in 4D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                convex_hull_test(4, &progress);
        }
        {
                *test_name = "Self-Test, Convex Hull in 5D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                convex_hull_test(5, &progress);
        }
        {
                *test_name = "Self-Test, 1-Manifold Reconstruction in 2D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                reconstruction_test(2, &progress);
        }
        {
                *test_name = "Self-Test, 2-Manifold Reconstruction in 3D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                reconstruction_test(3, &progress);
        }
}

void self_test_extended(IProgressRatioList* progress_ratio_list, std::string* test_name)
{
        {
                self_test_required(progress_ratio_list, test_name);
        }
        {
                *test_name = "Self-Test, 3-Manifold Reconstruction in 4D";
                ProgressRatio progress(progress_ratio_list, *test_name);
                reconstruction_test(4, &progress);
        }
}
}

void self_test(SelfTestType test_type, IProgressRatioList* progress_ratio_list, std::string* test_name)
{
        switch (test_type)
        {
        case SelfTestType::Required:
                self_test_required(progress_ratio_list, test_name);
                break;
        case SelfTestType::Extended:
                self_test_extended(progress_ratio_list, test_name);
                break;
        }
}
