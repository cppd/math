/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "com/spotlight.h"

#include <src/com/random/pcg.h>
#include <src/geometry/spatial/hyperplane_ball.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
class BallLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        geometry::spatial::HyperplaneBall<N, T> ball_;
        Color radiance_;
        T pdf_;
        T area_;
        std::array<numerical::Vector<N, T>, N - 1> vectors_;
        std::optional<com::Spotlight<N, T>> spotlight_;

        void init(const numerical::Vector<N, T>& scene_center, T scene_radius) override;

        [[nodiscard]] bool visible(const numerical::Vector<N, T>& point) const;
        [[nodiscard]] numerical::Vector<N, T> sample_location(PCG& engine) const;
        [[nodiscard]] Color radiance(T cos) const;

        [[nodiscard]] LightSourceArriveSample<N, T, Color> arrive_sample(
                PCG& engine,
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n) const override;

        [[nodiscard]] LightSourceArriveInfo<T, Color> arrive_info(
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& l) const override;

        [[nodiscard]] LightSourceLeaveSample<N, T, Color> leave_sample(PCG& engine) const override;

        [[nodiscard]] T leave_pdf_pos(const numerical::Vector<N, T>& dir) const override;
        [[nodiscard]] T leave_pdf_dir(const numerical::Vector<N, T>& dir) const override;

        [[nodiscard]] std::optional<Color> leave_radiance(const numerical::Vector<N, T>& dir) const override;

        [[nodiscard]] Color power() const override;

        [[nodiscard]] bool is_delta() const override;

        [[nodiscard]] bool is_infinite_area() const override;

public:
        BallLight(
                const numerical::Vector<N, T>& center,
                const numerical::Vector<N, T>& direction,
                std::type_identity_t<T> radius,
                const Color& radiance);

        BallLight(
                const numerical::Vector<N, T>& center,
                const numerical::Vector<N, T>& direction,
                std::type_identity_t<T> radius,
                const Color& radiance,
                std::type_identity_t<T> spotlight_falloff_start,
                std::type_identity_t<T> spotlight_width);

        void set_radiance_for_distance(T distance);
};
}
