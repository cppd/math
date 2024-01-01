/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>

#include <cstddef>
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
class LightDistribution final
{
        struct Base final
        {
                std::vector<LightDistributionSample<N, T, Color>> samples;
                std::unordered_map<const LightSource<N, T, Color>*, T> light_pdf;
        };

        [[nodiscard]] static std::shared_ptr<const Base> create_base(
                const std::vector<const LightSource<N, T, Color>*>& lights,
                const std::discrete_distribution<int>& distribution);

        std::discrete_distribution<int> distribution_;
        std::shared_ptr<const Base> base_;

public:
        explicit LightDistribution(const std::vector<const LightSource<N, T, Color>*>& lights);

        template <typename RandomEngine>
        [[nodiscard]] LightDistributionSample<N, T, Color> sample(RandomEngine& engine)
        {
                return base_->samples[distribution_(engine)];
        }

        [[nodiscard]] T pdf(const LightSource<N, T, Color>* const light) const
        {
                const auto iter = base_->light_pdf.find(light);
                if (iter != base_->light_pdf.cend())
                {
                        return iter->second;
                }
                error("Light not found in light distribution");
        }
};
}
