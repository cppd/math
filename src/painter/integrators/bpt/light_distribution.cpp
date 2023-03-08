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
#include <src/settings/instantiation.h>

#include <cmath>
#include <memory>
#include <random>
#include <unordered_map>
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
        return std::discrete_distribution<int>(powers.cbegin(), powers.cend());
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

template <std::size_t N, typename T, typename Color>
std::shared_ptr<const LightDistributionBase<N, T, Color>> create_light_distribution_base(
        const std::vector<const LightSource<N, T, Color>*>& lights,
        const std::discrete_distribution<int>& distribution)
{
        ASSERT(distribution.probabilities().size() == lights.size());

        const std::vector<T> probabilities = create_probabilities<T>(distribution);

        ASSERT(probabilities.size() == lights.size());

        std::vector<LightDistributionSample<N, T, Color>> samples;
        std::unordered_map<const LightSource<N, T, Color>*, T> light_pdf;

        samples.reserve(lights.size());
        light_pdf.reserve(lights.size());

        for (std::size_t i = 0; i < lights.size(); ++i)
        {
                samples.push_back(LightDistributionSample<N, T, Color>{.light = lights[i], .pdf = probabilities[i]});
                light_pdf[lights[i]] = probabilities[i];
        }

        return std::make_shared<const LightDistributionBase<N, T, Color>>(std::move(samples), std::move(light_pdf));
}
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::vector<LightDistribution<N, T, Color>> create_light_distributions(
        const Scene<N, T, Color>& scene,
        const unsigned count)
{
        const auto& lights = scene.light_sources();

        const std::discrete_distribution<int> distribution = create_distribution(lights);

        const std::shared_ptr<const LightDistributionBase<N, T, Color>> base =
                create_light_distribution_base(lights, distribution);

        std::vector<LightDistribution<N, T, Color>> res;
        res.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
                res.emplace_back(base, distribution);
        }
        return res;
}

#define TEMPLATE(N, T, C)                                                              \
        template std::vector<LightDistribution<(N), T, C>> create_light_distributions( \
                const Scene<(N), T, C>&, unsigned);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
