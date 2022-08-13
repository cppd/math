/*
Copyright (C) 2017-2022 Topological Manifold

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

#pragma once

#include "../objects.h"

#include <src/com/alg.h>
#include <src/com/error.h>

#include <cmath>
#include <random>

namespace ns::painter::integrators
{
namespace light_distribution_implementation
{
template <std::size_t N, typename T, typename Color>
[[nodiscard]] decltype(auto) light_power(const LightSource<N, T, Color>& light)
{
        return light.power().luminance();
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::discrete_distribution<int> create_distribution(const Scene<N, T, Color>& scene)
{
        const auto& lights = scene.light_sources();
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
}

template <typename T>
struct LightDistributionInfo final
{
        int index;
        T pdf;
};

template <typename T>
class LightDistribution final
{
        std::discrete_distribution<int> distribution_;
        std::vector<T> probabilities_;

public:
        template <std::size_t N, typename Color>
        explicit LightDistribution(const Scene<N, T, Color>& scene)
                : distribution_(light_distribution_implementation::create_distribution(scene)),
                  probabilities_(light_distribution_implementation::create_probabilities<T>(distribution_))
        {
                ASSERT(probabilities_.size() == scene.light_sources().size());
        }

        template <typename RandomEngine>
        [[nodiscard]] LightDistributionInfo<T> sample(RandomEngine& engine)
        {
                const int index = distribution_(engine);
                return {.index = index, .pdf = probabilities_[index]};
        }
};
}
