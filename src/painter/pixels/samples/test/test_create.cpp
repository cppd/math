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
#include "../com/info.h"
#include "../create.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

namespace ns::painter::pixels::samples::test
{
namespace
{
template <typename Samples>
void check_not_empty(const Samples& samples, const int count)
{
        static constexpr auto COUNT = com::Info<typename Samples::value_type>::COUNT;

        if (!samples)
        {
                error("Error creating samples<" + to_string(COUNT) + "> from sample count " + to_string(count)
                      + ": no data");
        }

        if (samples->empty())
        {
                error("Error creating samples<" + to_string(COUNT) + "> from sample count " + to_string(count)
                      + ": empty");
        }

        if (!(samples->count() > 0 && samples->count() <= COUNT))
        {
                error("Error creating samples<" + to_string(COUNT) + "> from sample count " + to_string(count)
                      + ": sample count = " + to_string(samples->count()));
        }
}

template <typename Samples>
void check_not_empty_not_full(const Samples& samples, const int count)
{
        static constexpr auto COUNT = com::Info<typename Samples::value_type>::COUNT;

        check_not_empty(samples, count);

        if (samples->full())
        {
                error("Error creating samples<" + to_string(COUNT) + "> from sample count " + to_string(count)
                      + ": full");
        }
}

template <typename Samples>
void check_full(const Samples& samples, const int count)
{
        static constexpr auto COUNT = com::Info<typename Samples::value_type>::COUNT;

        check_not_empty(samples, count);

        if (!samples->full())
        {
                error("Error creating samples<" + to_string(COUNT) + "> from sample count " + to_string(count)
                      + ": not full");
        }
}

template <typename C>
void test_background()
{
        using Colors = std::vector<std::optional<C>>;
        using Weights = std::vector<typename C::DataType>;

        {
                const Colors colors = {};
                const Weights weights = {};
                const auto samples = create_background_samples<2>(colors, weights);
                if (samples)
                {
                        error("Error creating empty samples");
                }
        }
        {
                const Colors colors = {{}};
                const Weights weights = {1};
                const auto samples = create_background_samples<2>(colors, weights);
                check_not_empty_not_full(samples, 1);
                compare_weights({1}, *samples);
        }
        {
                const Colors colors = {{}, {}};
                const Weights weights = {2, 1};
                const auto samples = create_background_samples<2>(colors, weights);
                check_not_empty_not_full(samples, 2);
                compare_weights({1, 2}, *samples);
        }
        {
                const Colors colors = {{}, C(1), {}, {}};
                const Weights weights = {3, 100, 1, 2};
                const auto samples = create_background_samples<2>(colors, weights);
                check_full(samples, 3);
                compare_weights({1, 3}, *samples);
                compare_weight_sum(2, *samples);
        }
        {
                const Colors colors = {C(1), {}, {}, {}, C(1), {}};
                const Weights weights = {100, 3, 2, 4, 100, 1};
                const auto samples = create_background_samples<2>(colors, weights);
                check_full(samples, 4);
                compare_weights({1, 4}, *samples);
                compare_weight_sum(2 + 3, *samples);
        }
        {
                const Colors colors = {C(1), {}, {}, {}, C(1), {}};
                const Weights weights = {100, 3, 2, 4, 100, 1};
                const auto samples = create_background_samples<4>(colors, weights);
                check_not_empty_not_full(samples, 4);
                compare_weights({1, 2, 3, 4}, *samples);
        }
        {
                const Colors colors = {C(1), {}, {}, {}, C(1), {}, C(1), {}};
                const Weights weights = {100, 3, 2, 4, 100, 5, 100, 1};
                const auto samples = create_background_samples<4>(colors, weights);
                check_full(samples, 5);
                compare_weights({1, 2, 4, 5}, *samples);
                compare_weight_sum(3, *samples);
        }
        {
                const Colors colors = {C(1), {}, {}, {}, C(1), {}, C(1), {}, {}};
                const Weights weights = {100, 3, 2, 4, 100, 5, 100, 1, 6};
                const auto samples = create_background_samples<4>(colors, weights);
                check_full(samples, 6);
                compare_weights({1, 2, 5, 6}, *samples);
                compare_weight_sum(3 + 4, *samples);
        }
}

template <typename C>
void test_color()
{
        using T = C::DataType;
        using Colors = std::vector<std::optional<C>>;
        using Weights = std::vector<typename C::DataType>;

        const auto scc = [](const C& c)
        {
                return sample_color_contribution(c);
        };

        {
                const Colors colors = {};
                const Weights weights = {};
                const auto samples = create_color_samples<2>(colors, weights);
                if (samples)
                {
                        error("Error creating empty samples");
                }
        }
        {
                const Colors colors = {C(0.5)};
                const Weights weights = {1};
                const auto samples = create_color_samples<2>(colors, weights);
                check_not_empty_not_full(samples, 1);
                compare_colors({C(0.5)}, *samples);
                compare_weights({1}, *samples);
                compare_contributions({1 * scc(C(0.5))}, *samples);
        }
        {
                const Colors colors = {C(0.5), C(0.25)};
                const Weights weights = {1, 1.1};
                const auto samples = create_color_samples<2>(colors, weights);
                check_not_empty_not_full(samples, 2);
                compare_colors({T{1.1} * C(0.25), T{1} * C(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions({T{1.1} * scc(C(0.25)), T{1} * scc(C(0.5))}, *samples);
        }
        {
                const Colors colors = {C(0.5), C(0.125), {}, C(0.25)};
                const Weights weights = {1, 1.1, 10, 1.2};
                const auto samples = create_color_samples<2>(colors, weights);
                check_full(samples, 3);
                compare_colors({T{1.1} * C(0.125), T{1} * C(0.5)}, *samples);
                compare_weights({1.1, 1}, *samples);
                compare_contributions({T{1.1} * scc(C(0.125)), T{1} * scc(C(0.5))}, *samples);
                compare_color_sum(T{1.2} * C(0.25), *samples);
                compare_weight_sum(1.2, *samples);
        }
        {
                const Colors colors = {{}, C(1), {}, C(0.25), C(0.5), C(0.125)};
                const Weights weights = {10, 1, 10, 1.1, 1.2, 1.3};
                const auto samples = create_color_samples<2>(colors, weights);
                check_full(samples, 4);
                compare_colors({T{1.3} * C(0.125), T{1} * C(1)}, *samples);
                compare_weights({1.3, 1}, *samples);
                compare_contributions({T{1.3} * scc(C(0.125)), T{1} * scc(C(1))}, *samples);
                compare_color_sum(T{1.1} * C(0.25) + T{1.2} * C(0.5), *samples);
                compare_weight_sum(T{1.1} + T{1.2}, *samples);
        }
        {
                const Colors colors = {{}, C(1), {}, C(0.25), C(0.5), C(0.125)};
                const Weights weights = {10, 1, 10, 1.1, 1.2, 1.3};
                const auto samples = create_color_samples<4>(colors, weights);
                check_not_empty_not_full(samples, 4);
                compare_colors({T{1.3} * C(0.125), T{1.1} * C(0.25), T{1.2} * C(0.5), T{1} * C(1)}, *samples);
                compare_weights({1.3, 1.1, 1.2, 1}, *samples);
                compare_contributions(
                        {T{1.3} * scc(C(0.125)), T{1.1} * scc(C(0.25)), T{1.2} * scc(C(0.5)), T{1} * scc(C(1))},
                        *samples);
        }
        {
                const Colors colors = {{}, C(1), {}, C(0.25), C(0.5), C(0.125), {}, C(1.0 / 16), {}};
                const Weights weights = {10, 1, 10, 1.1, 1.2, 1.3, 10, 1.4, 10};
                const auto samples = create_color_samples<4>(colors, weights);
                check_full(samples, 5);
                compare_colors({T{1.4} * C(1.0 / 16), T{1.3} * C(0.125), T{1.2} * C(0.5), T{1} * C(1)}, *samples);
                compare_weights({1.4, 1.3, 1.2, 1}, *samples);
                compare_contributions(
                        {T{1.4} * scc(C(1.0 / 16)), T{1.3} * scc(C(0.125)), T{1.2} * scc(C(0.5)), T{1} * scc(C(1))},
                        *samples);
                compare_color_sum(T{1.1} * C(0.25), *samples);
                compare_weight_sum(T{1.1}, *samples);
        }
        {
                const Colors colors = {{}, C(1), {}, C(0.25), C(0.5), C(0.125), {}, C(1.0 / 32), {}, C(1.0 / 16)};
                const Weights weights = {10, 1, 10, 1.1, 1.2, 1.3, 10, 1.4, 10, 1.5};
                const auto samples = create_color_samples<4>(colors, weights);
                check_full(samples, 6);
                compare_colors({T{1.4} * C(1.0 / 32), T{1.5} * C(1.0 / 16), T{1.2} * C(0.5), T{1} * C(1)}, *samples);
                compare_weights({1.4, 1.5, 1.2, 1}, *samples);
                compare_contributions(
                        {T{1.4} * scc(C(1.0 / 32)), T{1.5} * scc(C(1.0 / 16)), T{1.2} * scc(C(0.5)), T{1} * scc(C(1))},
                        *samples);
                compare_color_sum(T{1.3} * C(0.125) + T{1.1} * C(0.25), *samples);
                compare_weight_sum(T{1.3} + T{1.1}, *samples);
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
