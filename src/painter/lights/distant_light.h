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

#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
class DistantLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        LightSourceLeaveSample<N, T, Color> leave_sample_;
        LightSourceArriveSample<N, T, Color> arrive_sample_;
        std::array<Vector<N, T>, N - 1> vectors_;
        T area_;

        void init(const Vector<N, T>& scene_center, T scene_radius) override;

        [[nodiscard]] LightSourceArriveSample<N, T, Color> arrive_sample(PCG& engine, const Vector<N, T>& point)
                const override;

        [[nodiscard]] LightSourceArriveInfo<T, Color> arrive_info(const Vector<N, T>& point, const Vector<N, T>& l)
                const override;

        [[nodiscard]] LightSourceLeaveSample<N, T, Color> leave_sample(PCG& engine) const override;

        [[nodiscard]] T leave_pdf_pos(const Vector<N, T>& point, const Vector<N, T>& dir) const override;
        [[nodiscard]] T leave_pdf_dir(const Vector<N, T>& point, const Vector<N, T>& dir) const override;

        [[nodiscard]] std::optional<Color> leave_radiance(
                const Ray<N, T>& ray_to_light,
                const std::optional<T>& distance) const override;

        [[nodiscard]] Color power() const override;

        [[nodiscard]] bool is_delta() const override;

public:
        DistantLight(const Vector<N, T>& direction, const Color& radiance);
};
}
