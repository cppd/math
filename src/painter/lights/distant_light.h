/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/numerical/vec.h>

#include <type_traits>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class DistantLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        LightSourceSample<N, T, Color> sample_;

public:
        DistantLight(const Vector<N, T>& direction, const Color& color)
        {
                sample_.l = direction.normalized();
                sample_.pdf = 1;
                sample_.radiance = color;
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& /*random_engine*/, const Vector<N, T>& /*point*/)
                const override
        {
                return sample_;
        }

        LightSourceInfo<T, Color> info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const override
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

        bool is_delta() const override
        {
                return true;
        }
};
}
