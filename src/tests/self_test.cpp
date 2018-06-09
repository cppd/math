/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/names.h"
#include "com/string/str.h"
#include "geometry/test/test_convex_hull.h"
#include "geometry/test/test_reconstruction.h"
#include "gpu_2d/dft/test/test_dft.h"
#include "path_tracing/shapes/test/test_mesh.h"
#include "path_tracing/space/test/test_parallelotope.h"

namespace
{
std::string space_name_upper(int d)
{
        return to_upper_first_letters(space_name(d));
}

template <typename T>
void self_test_essential(IProgressRatioList* progress_ratio_list, const T& catch_all)
{
        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, DFT in " + space_name_upper(2);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_dft(&progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Parallelotope in " + space_name_upper(2);

                ProgressRatio progress(progress_ratio_list, *test_name);
                progress.set(0);
                test_parallelotope(2);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Parallelotope in " + space_name_upper(3);

                ProgressRatio progress(progress_ratio_list, *test_name);
                progress.set(0);
                test_parallelotope(3);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Parallelotope in " + space_name_upper(4);

                ProgressRatio progress(progress_ratio_list, *test_name);
                progress.set(0);
                test_parallelotope(4);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Mesh in " + space_name_upper(3);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_mesh(3, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Mesh in " + space_name_upper(4);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_mesh(4, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Convex Hull in " + space_name_upper(2);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_convex_hull(2, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Convex Hull in " + space_name_upper(3);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_convex_hull(3, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Convex Hull in " + space_name_upper(4);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_convex_hull(4, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, 1-Manifold Reconstruction in " + space_name_upper(2);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_reconstruction(2, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, 2-Manifold Reconstruction in " + space_name_upper(3);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_reconstruction(3, &progress);
        });
}

template <typename T>
void self_test_extended(IProgressRatioList* progress_ratio_list, const T& catch_all)
{
        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Convex Hull in " + space_name_upper(5);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_convex_hull(5, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Mesh in " + space_name_upper(5);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_mesh(5, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, Mesh in " + space_name_upper(6);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_mesh(6, &progress);
        });

        catch_all([&](std::string* test_name) {
                *test_name = "Self-Test, 3-Manifold Reconstruction in " + space_name_upper(4);

                ProgressRatio progress(progress_ratio_list, *test_name);
                test_reconstruction(4, &progress);
        });
}
}

void self_test(SelfTestType test_type, IProgressRatioList* progress_ratio_list,
               std::function<void(const std::exception_ptr& ptr, const std::string& msg)> exception_handler)
{
        auto catch_all = [&](const auto& function) noexcept
        {
                std::string message = "Self-Test";
                try
                {
                        function(&message);
                }
                catch (...)
                {
                        exception_handler(std::current_exception(), message);
                }
        };

        switch (test_type)
        {
        case SelfTestType::Essential:
                self_test_essential(progress_ratio_list, catch_all);
                break;
        case SelfTestType::Extended:
                self_test_essential(progress_ratio_list, catch_all);
                self_test_extended(progress_ratio_list, catch_all);
                break;
        }
}
