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

#include "light_distribution.h"

#include <src/color/color.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstddef>
#include <memory>
#include <random>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace
{
constexpr bool EQUAL_LIGHT_POWER = true;

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T light_power(const LightSource<N, T, Color>& light)
{
        if (EQUAL_LIGHT_POWER)
        {
                return 1;
        }
        return light.power().luminance();
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::discrete_distribution<int> create_distribution(
        const std::vector<const LightSource<N, T, Color>*>& lights)
{
        if (lights.empty())
        {
                error("No light sources");
        }

        std::vector<T> powers;
        powers.reserve(lights.size());
        for (const LightSource<N, T, Color>* const light : lights)
        {
                powers.push_back(light_power(*light));
        }

        return {powers.cbegin(), powers.cend()};
}

template <typename T>
[[nodiscard]] std::vector<T> create_probabilities(const std::discrete_distribution<int>& distribution)
{
        const std::vector<double> probabilities = distribution.probabilities();

        const auto sum = add_all<double>(probabilities);
        if (!(std::abs(sum - 1) < 1e-10))
        {
                error("Probability sum " + to_string(sum) + " is not equal to 1");
        }

        return {probabilities.cbegin(), probabilities.cend()};
}
}

template <std::size_t N, typename T, typename Color>
std::shared_ptr<const typename LightDistribution<N, T, Color>::Base> LightDistribution<N, T, Color>::create_base(
        const std::vector<const LightSource<N, T, Color>*>& lights,
        const std::discrete_distribution<int>& distribution)
{
        ASSERT(distribution.probabilities().size() == lights.size());

        const std::vector<T> probabilities = create_probabilities<T>(distribution);
        ASSERT(probabilities.size() == lights.size());

        auto base = std::make_shared<Base>();

        base->samples.reserve(lights.size());
        base->light_pdf.reserve(lights.size());

        for (std::size_t i = 0; i < lights.size(); ++i)
        {
                base->samples.push_back({.light = lights[i], .pdf = probabilities[i]});
                base->light_pdf[lights[i]] = probabilities[i];
        }

        return base;
}

template <std::size_t N, typename T, typename Color>
LightDistribution<N, T, Color>::LightDistribution(const std::vector<const LightSource<N, T, Color>*>& lights)
        : distribution_(create_distribution(lights)),
          base_(create_base(lights, distribution_))
{
}

#define TEMPLATE(N, T, C) \
        template LightDistribution<(N), T, C>::LightDistribution(const std::vector<const LightSource<(N), T, C>*>&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
