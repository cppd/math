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

#include "compare.h"

#include "../../color_contribution.h"
#include "../background_create.h"
#include "../color_create.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/test/test.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
template <typename C>
void test_background()
{
        using T = typename C::DataType;

        std::vector<std::optional<C>> colors;
        std::vector<T> weights;

        {
                colors = {};
                weights = {};
                const auto samples = create_background_samples(colors, weights);
                if (samples)
                {
                        error("Error creating empty");
                }
        }
        {
                colors = {{}};
                weights = {1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 1 sample");
                }
                compare_weights({1}, *samples);
        }
        {
                colors = {{}, {}};
                weights = {2, 1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 2 samples");
                }
                compare_weights({1, 2}, *samples);
        }
        {
                colors = {{}, C(1), {}, {}};
                weights = {3, 100, 1, 2};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 3 samples");
                }
                compare_weights({1, 3}, *samples);
                compare_weight_sum(2, *samples);
        }
        {
                colors = {C(1), {}, {}, {}, C(1), {}};
                weights = {100, 3, 2, 4, 100, 1};
                const auto samples = create_background_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 4 samples");
                }
                compare_weights({1, 4}, *samples);
                compare_weight_sum(2 + 3, *samples);
        }
}

template <typename C>
void test_color()
{
        using T = typename C::DataType;

        std::vector<std::optional<C>> colors;
        std::vector<T> weights;

        {
                colors = {};
                weights = {};
                const auto samples = create_color_samples(colors, weights);
                if (samples)
                {
                        error("Error creating empty");
                }
        }
        {
                colors = {C(0.5)};
                weights = {1};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 1 sample");
                }
                compare_colors({C(0.5)}, *samples);
                compare_weights({1}, *samples);
                compare_contributions({1 * sample_color_contribution(C(0.5))}, *samples);
        }
        {
                colors = {C(0.5), C(0.25)};
                weights = {1, 1.1};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || samples->full())
                {
                        error("Error creating samples from 2 samples");
                }
                compare_colors({T{1.1} * C(0.25), T{1} * C(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions(
                        {T{1.1} * sample_color_contribution(C(0.25)), T{1} * sample_color_contribution(C(0.5))},
                        *samples);
        }
        {
                colors = {C(0.5), C(0.125), {}, C(0.25)};
                weights = {1, 1.1, 10, 1.2};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 3 samples");
                }
                compare_colors({T{1.1} * C(0.125), T{1} * C(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions(
                        {T{1.1} * sample_color_contribution(C(0.125)), T{1} * sample_color_contribution(C(0.5))},
                        *samples);
                compare_color_sum(T{1.2} * C(0.25), *samples);
                compare_weight_sum(1.2, *samples);
        }
        {
                colors = {{}, C(1), {}, C(0.25), C(0.5), C(0.125)};
                weights = {10, 1, 10, 1.1, 1.2, 1.3};
                const auto samples = create_color_samples(colors, weights);
                if (!samples || samples->empty() || !samples->full())
                {
                        error("Error creating samples from 4 samples");
                }
                compare_colors({T{1.3} * C(0.125), T{1} * C(1)}, *samples);
                compare_weights({1.3, 1}, *samples);
                compare_contributions(
                        {T{1.3} * sample_color_contribution(C(0.125)), T{1} * sample_color_contribution(C(1))},
                        *samples);
                compare_color_sum(T{1.1} * C(0.25) + T{1.2} * C(0.5), *samples);
                compare_weight_sum(T{1.1} + T{1.2}, *samples);
        }
}

template <typename C>
void test()
{
        test_background<C>();
        test_color<C>();
}

void test_create()
{
        LOG("Test pixel create samples");
        test<color::Color>();
        test<color::Spectrum>();
        LOG("Test pixel create samples passed");
}

TEST_SMALL("Pixel Create Samples", test_create)
}
}
