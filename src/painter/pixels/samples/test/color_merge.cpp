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

#include "color_merge.h"

#include "compare.h"

#include "../color_merge.h"

#include <src/color/color.h>
#include <src/com/error.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
template <typename T>
void test(const ColorSamples<color::Color>& a, const ColorSamples<color::Color>& b, const T& test)
{
        test(a, b);
        test(b, a);
}
}

void test_color_merge()
{
        using C = color::Color;
        using S = ColorSamples<color::Color>;

        test(S(), S(),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     if (!s.empty())
                     {
                             error("Error merging empty");
                     }
             });

        test(S({C(1), C(2)}, {1, 2}, {1, 2}, 2), S(),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     if (s.empty() || s.full())
                     {
                             error("Error merging empty and non-empty");
                     }
                     compare_colors({C(1), C(2)}, s);
                     compare_weights({1, 2}, s);
                     compare_contributions({1, 2}, s);
             });

        test(S({C(1)}, {1}, {1}, 1), S({C(1), C(2)}, {1, 2}, {1, 2}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     compare_colors({C(1), C(2)}, s);
                     compare_weights({1, 2}, s);
                     compare_contributions({1, 2}, s);
                     compare_color_sum(C(1), s);
                     compare_weight_sum(1, s);
             });

        test(S({C(1), C(2)}, {1, 2}, {1, 2}, 2), S({C(1), C(3)}, {1, 3}, {1, 3}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     compare_colors({C(1), C(3)}, s);
                     compare_weights({1, 3}, s);
                     compare_contributions({1, 3}, s);
                     compare_color_sum(C(3), s);
                     compare_weight_sum(3, s);
             });

        test(S(C(1), {C(2), C(3)}, 1, {2, 3}, {2, 3}), S({C(1)}, {1}, {1}, 1),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     compare_colors({C(1), C(3)}, s);
                     compare_weights({1, 3}, s);
                     compare_contributions({1, 3}, s);
                     compare_color_sum(C(3), s);
                     compare_weight_sum(3, s);
             });

        test(S(C(1), {C(2), C(3)}, 1, {2, 3}, {2, 3}), S({C(1), C(2)}, {1, 2}, {1, 2}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     compare_colors({C(1), C(3)}, s);
                     compare_weights({1, 3}, s);
                     compare_contributions({1, 3}, s);
                     compare_color_sum(C(5), s);
                     compare_weight_sum(5, s);
             });

        test(S(C(1), {C(2), C(4)}, 1, {2, 4}, {2, 4}), S(C(3), {C(1), C(3)}, 3, {1, 3}, {1, 3}),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_color_samples<color::Color>(a, b);
                     compare_colors({C(1), C(4)}, s);
                     compare_weights({1, 4}, s);
                     compare_contributions({1, 4}, s);
                     compare_color_sum(C(9), s);
                     compare_weight_sum(9, s);
             });
}
}
