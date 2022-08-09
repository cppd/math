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
#include "com/spot_light.h"

#include <src/geometry/spatial/hyperplane_ball.h>

#include <array>
#include <optional>
#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
class BallLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        geometry::HyperplaneBall<N, T> ball_;
        Color radiance_;
        T pdf_;
        std::array<Vector<N, T>, N - 1> vectors_;
        std::optional<com::SpotLight<N, T>> spotlight_;

        [[nodiscard]] bool visible(const Vector<N, T>& point) const;
        [[nodiscard]] Vector<N, T> sample_location(PCG& engine) const;
        [[nodiscard]] Color radiance(T cos) const;

        [[nodiscard]] LightSourceSample<N, T, Color> sample(PCG& engine, const Vector<N, T>& point) const override;

        [[nodiscard]] LightSourceInfo<T, Color> info(const Vector<N, T>& point, const Vector<N, T>& l) const override;

        [[nodiscard]] LightSourceSampleEmit<N, T, Color> sample_emit(PCG& engine) const override;

        [[nodiscard]] Color power() const override;

        [[nodiscard]] bool is_delta() const override;

public:
        BallLight(
                const Vector<N, T>& center,
                const Vector<N, T>& direction,
                std::type_identity_t<T> radius,
                const Color& radiance);

        BallLight(
                const Vector<N, T>& center,
                const Vector<N, T>& direction,
                std::type_identity_t<T> radius,
                const Color& radiance,
                std::type_identity_t<T> spotlight_falloff_start,
                std::type_identity_t<T> spotlight_width);

        void set_radiance_for_distance(T distance);
};
}
