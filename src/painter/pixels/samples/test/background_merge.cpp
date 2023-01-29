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

#include "background_merge.h"

#include "../background_merge.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
void compare_weights(const std::vector<color::Color::DataType>& weights, const BackgroundSamples<color::Color>& samples)
{
        if (!(samples.count() == weights.size()))
        {
                error("Error sample count " + to_string(samples.count()) + " is not equal to weight count "
                      + to_string(weights.size()));
        }

        for (std::size_t i = 0; i < weights.size(); ++i)
        {
                if (!(samples.weight(i) == weights[i]))
                {
                        error("Sample weight " + to_string(samples.weight(i)) + " is not equal to "
                              + to_string(weights[i]) + ", index " + to_string(i));
                }
        }
}

void compare_weight_sum(const color::Color::DataType weight_sum, const BackgroundSamples<color::Color>& samples)
{
        if (!(samples.weight_sum() == weight_sum))
        {
                error("Sample weight sum " + to_string(samples.weight_sum()) + " is not equal to "
                      + to_string(weight_sum));
        }
}

template <typename T>
void test(const BackgroundSamples<color::Color>& a, const BackgroundSamples<color::Color>& b, const T& test)
{
        test(a, b);
        test(b, a);
}
}

void test_background_merge()
{
        using S = BackgroundSamples<color::Color>;

        test(S(), S(),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     if (!s.empty())
                     {
                             error("Error merging empty");
                     }
             });

        test(S({1, 2}, 2), S(),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     if (s.empty() || s.full())
                     {
                             error("Error merging empty and non-empty");
                     }
                     compare_weights({1, 2}, s);
             });

        test(S({1}, 1), S({1, 2}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     compare_weights({1, 2}, s);
                     compare_weight_sum(1, s);
             });

        test(S({1, 2}, 2), S({1, 3}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     compare_weights({1, 3}, s);
                     compare_weight_sum(3, s);
             });

        test(S(1, {2, 3}), S({1}, 1),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     compare_weights({1, 3}, s);
                     compare_weight_sum(3, s);
             });

        test(S(1, {2, 3}), S({1, 2}, 2),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     compare_weights({1, 3}, s);
                     compare_weight_sum(5, s);
             });

        test(S(1, {2, 4}), S(3, {1, 3}),
             [](const auto& a, const auto& b)
             {
                     const auto s = merge_background_samples<color::Color>(a, b);
                     compare_weights({1, 4}, s);
                     compare_weight_sum(9, s);
             });
}
}
