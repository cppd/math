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

#pragma once

#include "../../objects.h"

#include <src/com/alg.h>
#include <src/com/error.h>

#include <cmath>
#include <memory>
#include <random>
#include <unordered_map>

namespace ns::painter::integrators::bpt
{
namespace light_distribution_implementation
{
inline constexpr bool EQUAL_LIGHT_POWER = true;

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
        std::vector<LightDistributionSample<N, T, Color>> samples_;
        std::unordered_map<const LightSource<N, T, Color>*, T> light_pdf_;

public:
        LightDistributionBase(
                const std::vector<const LightSource<N, T, Color>*>& lights,
                const std::discrete_distribution<int>& distribution)
        {
                ASSERT(distribution.probabilities().size() == lights.size());

                const std::vector<T> probabilities =
                        light_distribution_implementation::create_probabilities<T>(distribution);

                ASSERT(probabilities.size() == lights.size());

                samples_.reserve(lights.size());
                light_pdf_.reserve(lights.size());
                for (std::size_t i = 0; i < lights.size(); ++i)
                {
                        samples_.push_back(
                                LightDistributionSample<N, T, Color>{.light = lights[i], .pdf = probabilities[i]});
                        light_pdf_[lights[i]] = probabilities[i];
                }
        }

        [[nodiscard]] LightDistributionSample<N, T, Color> sample(const int index) const
        {
                return samples_[index];
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
};

template <std::size_t N, typename T, typename Color>
class LightDistribution final
{
        std::shared_ptr<const LightDistributionBase<N, T, Color>> base_;
        std::discrete_distribution<int> distribution_;

public:
        LightDistribution(
                std::shared_ptr<const LightDistributionBase<N, T, Color>> base,
                std::discrete_distribution<int> distribution)
                : base_(std::move(base)),
                  distribution_(std::move(distribution))
        {
        }

        template <typename RandomEngine>
        [[nodiscard]] LightDistributionSample<N, T, Color> sample(RandomEngine& engine)
        {
                return base_->sample(distribution_(engine));
        }

        [[nodiscard]] T pdf(const LightSource<N, T, Color>* const light) const
        {
                return base_->pdf(light);
        }
};

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::vector<LightDistribution<N, T, Color>> create_light_distributions(
        const Scene<N, T, Color>& scene,
        const unsigned count)
{
        const auto& lights = scene.light_sources();

        const std::discrete_distribution<int> distribution =
                light_distribution_implementation::create_distribution(lights);

        const auto base = std::make_shared<const LightDistributionBase<N, T, Color>>(lights, distribution);

        std::vector<LightDistribution<N, T, Color>> res;
        res.reserve(count);
        for (unsigned i = 0; i < count; ++i)
        {
                res.emplace_back(base, distribution);
        }
        return res;
}
}
