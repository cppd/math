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

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace ns::painter::integrators::bpt
{
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
                std::vector<LightDistributionSample<N, T, Color>> samples,
                std::unordered_map<const LightSource<N, T, Color>*, T> light_pdf)
                : samples_(std::move(samples)),
                  light_pdf_(std::move(light_pdf))
        {
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
        unsigned count);
}
