/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/exception.h>
#include <src/com/names.h>
#include <src/geometry/core/test/test_convex_hull.h>
#include <src/geometry/reconstruction/test/test_reconstruction.h>
#include <src/gpu/dft/test/test_dft.h>
#include <src/numerical/test/test_complement.h>
#include <src/numerical/test/test_eigen.h>
#include <src/numerical/test/test_normal.h>
#include <src/numerical/test/test_simplex.h>
#include <src/painter/shapes/test/test_mesh.h>
#include <src/painter/space/test/test_parallelotope.h>
#include <src/utility/string/str.h>

namespace test
{
namespace
{
std::string space_name_upper(int d)
{
        return to_upper_first_letters(space_name(d));
}

void self_test_essential(ProgressRatios* progress_ratios)
{
        std::string s;

        s = "Self-Test, Complement";
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                numerical::test_complement(&progress);
        });

        s = "Self-Test, Eigen";
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                numerical::test_eigen(&progress);
        });

        s = "Self-Test, Normal";
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                numerical::test_normal(&progress);
        });

        s = "Self-Test, Simplex";
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                numerical::test_simplex(&progress);
        });

        s = "Self-Test, DFT in " + space_name_upper(2);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                gpu::dft::test(&progress);
        });

        s = "Self-Test, Parallelotope in " + space_name_upper(2);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                progress.set(0);
                painter::test_parallelotope(2);
        });

        s = "Self-Test, Parallelotope in " + space_name_upper(3);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                progress.set(0);
                painter::test_parallelotope(3);
        });

        s = "Self-Test, Parallelotope in " + space_name_upper(4);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                progress.set(0);
                painter::test_parallelotope(4);
        });

        s = "Self-Test, Mesh in " + space_name_upper(3);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                painter::test_mesh(3, &progress);
        });

        s = "Self-Test, Mesh in " + space_name_upper(4);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                painter::test_mesh(4, &progress);
        });

        s = "Self-Test, Convex Hull in " + space_name_upper(2);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_convex_hull(2, &progress);
        });

        s = "Self-Test, Convex Hull in " + space_name_upper(3);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_convex_hull(3, &progress);
        });

        s = "Self-Test, Convex Hull in " + space_name_upper(4);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_convex_hull(4, &progress);
        });

        s = "Self-Test, 1-Manifold Reconstruction in " + space_name_upper(2);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_reconstruction(2, &progress);
        });

        s = "Self-Test, 2-Manifold Reconstruction in " + space_name_upper(3);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_reconstruction(3, &progress);
        });
}

void self_test_extended(ProgressRatios* progress_ratios)
{
        std::string s;

        s = "Self-Test, Convex Hull in " + space_name_upper(5);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_convex_hull(5, &progress);
        });

        s = "Self-Test, Mesh in " + space_name_upper(5);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                painter::test_mesh(5, &progress);
        });

        s = "Self-Test, Mesh in " + space_name_upper(6);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                painter::test_mesh(6, &progress);
        });

        s = "Self-Test, 3-Manifold Reconstruction in " + space_name_upper(4);
        catch_all(s, [&]() {
                ProgressRatio progress(progress_ratios, s);
                geometry::test_reconstruction(4, &progress);
        });
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
