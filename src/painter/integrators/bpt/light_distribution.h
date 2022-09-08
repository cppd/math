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

#include "../../objects.h"

#include <src/com/alg.h>
#include <src/com/error.h>

#include <cmath>
#include <random>
#include <unordered_map>

namespace ns::painter::integrators::bpt
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

template <std::size_t N, typename T, typename Color>
struct LightDistributionSample final
{
        const LightSource<N, T, Color>* light;
        T pdf;
};

template <std::size_t N, typename T, typename Color>
class LightDistributionBase final
{
        template <std::size_t, typename, typename>
        friend class LightDistribution;

        std::discrete_distribution<int> distribution_;

        std::vector<LightDistributionSample<N, T, Color>> samples_;
        std::unordered_map<const LightSource<N, T, Color>*, T> light_pdf_;

        [[nodiscard]] const std::discrete_distribution<int>& distribution() const
        {
                return distribution_;
        }

        template <typename RandomEngine>
        [[nodiscard]] LightDistributionSample<N, T, Color> sample(
                std::discrete_distribution<int>& distribution,
                RandomEngine& engine) const
        {
                return samples_[distribution(engine)];
        }

        [[nodiscard]] T pdf(const LightSource<N, T, Color>* const light) const
        {
                const auto iter = light_pdf_.find(light);
                if (iter != light_pdf_.cend())
                {
                        return iter->second;
                }
                error("Light not found in light distribution");
        }

public:
        explicit LightDistributionBase(const Scene<N, T, Color>& scene)
                : distribution_(light_distribution_implementation::create_distribution(scene))
        {
                const std::vector<T> probabilities =
                        light_distribution_implementation::create_probabilities<T>(distribution_);

                const auto& lights = scene.light_sources();
                ASSERT(probabilities.size() == lights.size());

                samples_.reserve(lights.size());
                light_pdf_.reserve(lights.size());
                for (std::size_t i = 0; i < lights.size(); ++i)
                {
                        samples_.emplace_back(lights[i], probabilities[i]);
                        light_pdf_[lights[i]] = probabilities[i];
                }
        }
};

template <std::size_t N, typename T, typename Color>
class LightDistribution final
{
        const LightDistributionBase<N, T, Color>* base_;
        std::discrete_distribution<int> distribution_;

public:
        explicit LightDistribution(const LightDistributionBase<N, T, Color>* const base)
                : base_(base),
                  distribution_(base->distribution())
        {
        }

        template <typename RandomEngine>
        [[nodiscard]] LightDistributionSample<N, T, Color> sample(RandomEngine& engine)
        {
                return base_->sample(distribution_, engine);
        }

        [[nodiscard]] T pdf(const LightSource<N, T, Color>* const light) const
        {
                return base_->pdf(light);
        }
};
}
