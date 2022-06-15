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

#include "storage_scene.h"

#include "../lights/parallelotope_light.h"
#include "../lights/point_light.h"
#include "../lights/spot_light.h"
#include "../objects.h"
#include "../projectors/perspective_projector.h"
#include "../projectors/spherical_projector.h"
#include "../shapes/hyperplane_parallelotope.h"
#include "../shapes/parallelotope.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/color/colors.h>
#include <src/com/arrays.h>

#include <array>
#include <memory>
#include <tuple>

namespace ns::painter
{
namespace cornell_box_scene_implementation
{
template <std::size_t N, typename T, typename Color>
void create_shapes(
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>* const shapes)
{
        constexpr T BOX_SIZE = 0.16;
        constexpr T BOX_SPACE = 0.08;
        constexpr T NEAR = 0.9;
        constexpr T DEPTH = NEAR + 0.5 + BOX_SIZE + 2 * BOX_SPACE;

        constexpr T ALPHA = 1;
        constexpr T METALNESS = 0;
        constexpr T ROUGHNESS = 0.25;

        const Vector<N, T> org = [&]
        {
                Vector<N, T> res(0);
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res -= camera[i];
                }
                res *= T{0.5};
                res -= NEAR * camera[N - 1];
                res += center;
                return res;
        }();

        // Walls
        {
                std::array<Vector<N, T>, N> walls_vectors = camera;
                walls_vectors[N - 1] *= DEPTH;

                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        shapes->push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::RED), ALPHA, org,
                                del_elem(walls_vectors, i)));
                        shapes->push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::GREEN), ALPHA,
                                org + walls_vectors[i], del_elem(walls_vectors, i)));
                }
                shapes->push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::WHITE), ALPHA, org + walls_vectors[N - 1],
                        del_elem(walls_vectors, N - 1)));
        }

        // Box
        {
                Vector<N, T> box_org = org;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        box_org += (1 - BOX_SPACE - BOX_SIZE) * camera[i];
                }
                box_org += BOX_SPACE * camera[N - 2];
                box_org += (DEPTH - BOX_SPACE - BOX_SIZE) * camera[N - 1];

                std::array<Vector<N, T>, N> box_vectors;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        box_vectors[i] = BOX_SIZE * camera[i];
                }
                box_vectors[N - 2] = (1 - 2 * BOX_SPACE) * camera[N - 2];
                box_vectors[N - 1] = BOX_SIZE * camera[N - 1];

                shapes->push_back(std::make_unique<Parallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::MAGENTA), ALPHA, box_org, box_vectors));
        }
}

template <std::size_t N, typename T>
std::unique_ptr<Projector<N, T>> create_projector(
        const std::array<int, N - 1>& screen_size,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center)
{
        constexpr T POSITION = 1.3;

        const std::array<Vector<N, T>, N - 1> screen_axes = del_elem(camera, N - 1);
        const Vector<N, T> view_point = center - POSITION * camera[N - 1];

        switch (0)
        {
        case 0:
        {
                return std::make_unique<PerspectiveProjector<N, T>>(
                        view_point, camera[N - 1], screen_axes, 60, screen_size);
        }
        case 1:
        {
                return std::make_unique<SphericalProjector<N, T>>(
                        view_point, camera[N - 1], screen_axes, 70, screen_size);
        }
        }
}

template <std::size_t N, typename T, typename Color>
void create_light_sources(
        const Color& light,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center,
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>>* const light_sources)
{
        constexpr T FALLOFF_START = 60;
        constexpr T WIDTH = 72;

        switch (0)
        {
        case 0:
        {
                constexpr T SIZE = 0.1;
                constexpr T INTENSITY = power<N - 1>(T{8});

                Vector<N, T> org = center;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        org -= (SIZE / 2) * camera[i];
                }
                org += T{0.49} * camera[N - 2];
                org -= (SIZE / 2) * camera[N - 1];

                std::array<Vector<N, T>, N - 1> vectors;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        vectors[i] = SIZE * camera[i];
                }
                vectors[N - 2] = SIZE * camera[N - 1];

                Vector<N, T> direction = -camera[N - 2];

                light_sources->push_back(std::make_unique<const ParallelotopeLight<N, T, Color>>(
                        org, vectors, direction, INTENSITY * light, FALLOFF_START, WIDTH));

                break;
        }
        case 1:
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1.5;

                const Vector<N, T> org = center + T{0.49} * camera[N - 2];
                const Vector<N, T> direction = -camera[N - 2];

                light_sources->push_back(std::make_unique<const SpotLight<N, T, Color>>(
                        org, direction, light, UNIT_INTENSITY_DISTANCE, FALLOFF_START, WIDTH));

                break;
        }
        case 2:
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1;

                const Vector<N, T> org = center + T{0.45} * camera[N - 2];

                light_sources->push_back(
                        std::make_unique<const PointLight<N, T, Color>>(org, light, UNIT_INTENSITY_DISTANCE));

                break;
        }
        }
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_cornell_box_scene(
        const Color& light,
        const Color& background_light,
        const std::array<int, N - 1>& screen_size,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center,
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        progress::Ratio* const progress)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources;

        shapes.push_back(std::move(shape));

        create_shapes(camera, center, &shapes);

        create_light_sources(light, camera, center, &light_sources);

        std::unique_ptr<Projector<N, T>> projector = create_projector(screen_size, camera, center);

        return create_storage_scene<N, T>(
                background_light, /*clip_plane_equation*/ std::nullopt, std::move(projector), std::move(light_sources),
                std::move(shapes), progress);
}

template <std::size_t N, typename T>
std::tuple<std::array<Vector<N, T>, N>, Vector<N, T>> camera_and_center(const geometry::BoundingBox<N, T>& bb)
{
        const T size = bb.diagonal().norm() * T{1.5};

        std::array<Vector<N, T>, N> camera;
        for (std::size_t i = 0; i < N; ++i)
        {
                camera[i] = Vector<N, T>(0);
        }
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                camera[i][i] = size;
        }
        camera[N - 1][N - 1] = -size;

        Vector<N, T> center = bb.center();
        center[N - 2] += (size - (bb.max()[N - 2] - bb.min()[N - 2])) * T{0.5};

        return {camera, center};
}
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_cornell_box_scene(
        const Color& light,
        const Color& background_light,
        const std::array<int, N - 1>& screen_size,
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        progress::Ratio* const progress)
{
        static_assert(N >= 3);
        namespace impl = cornell_box_scene_implementation;

        const auto [camera, center] = impl::camera_and_center(shape->bounding_box());

        return impl::create_cornell_box_scene(
                light, background_light, screen_size, camera, center, std::move(shape), progress);
}
}
