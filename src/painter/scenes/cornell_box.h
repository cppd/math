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

#include "storage_scene.h"

#include "../lights/point_light.h"
#include "../objects.h"
#include "../projectors/perspective_projector.h"
#include "../projectors/spherical_projector.h"
#include "../shapes/hyperplane_parallelotope.h"
#include "../shapes/parallelotope.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/color/colors.h>
#include <src/com/arrays.h>

#include <memory>

namespace ns::painter
{
namespace cornell_box_scene_implementation
{
template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_cornell_box_scene(
        const Color& light,
        const Color& background_light,
        const std::array<int, N - 1>& screen_sizes,
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center)
{
        static_assert(N >= 3);

        //constexpr T LAMP_SIZE = 0.2;
        constexpr T BOX_SIZE = 0.16;
        constexpr T BOX_SPACE = 0.08;
        constexpr T CAMERA = 0.8;
        constexpr T NEAR = 0.7;
        constexpr T DEPTH = NEAR + 0.5 + BOX_SIZE + 2 * BOX_SPACE;

        constexpr T ALPHA = 1;
        constexpr T METALNESS = 0;
        constexpr T ROUGHNESS = 0.35;

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;
        std::vector<std::unique_ptr<const LightSource<N, T, Color>>> light_sources;

        shapes.push_back(std::move(shape));

        Vector<N, T> org(0);
        for (unsigned i = 0; i < N - 1; ++i)
        {
                org -= camera[i];
        }
        org *= T(0.5);
        org -= NEAR * camera[N - 1];
        org += center;

        // Walls
        {
                std::array<Vector<N, T>, N> walls_vectors = camera;
                walls_vectors[N - 1] *= DEPTH;

                for (unsigned i = 0; i < N - 1; ++i)
                {
                        shapes.push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::RED), ALPHA, org,
                                del_elem(walls_vectors, i)));
                        shapes.push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::GREEN), ALPHA,
                                org + walls_vectors[i], del_elem(walls_vectors, i)));
                }
                shapes.push_back(std::make_unique<HyperplaneParallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::WHITE), ALPHA, org + walls_vectors[N - 1],
                        del_elem(walls_vectors, N - 1)));
        }

        // Box
        {
                Vector<N, T> box_org = org;
                for (unsigned i = 0; i < N - 2; ++i)
                {
                        box_org += (1 - BOX_SPACE - BOX_SIZE) * camera[i];
                }
                box_org += BOX_SPACE * camera[N - 2];
                box_org += (DEPTH - BOX_SPACE - BOX_SIZE) * camera[N - 1];

                std::array<Vector<N, T>, N> box_vectors;
                for (unsigned i = 0; i < N - 2; ++i)
                {
                        box_vectors[i] = BOX_SIZE * camera[i];
                }
                box_vectors[N - 2] = (1 - 2 * BOX_SPACE) * camera[N - 2];
                box_vectors[N - 1] = BOX_SIZE * camera[N - 1];

                shapes.push_back(std::make_unique<Parallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::MAGENTA), ALPHA, box_org, box_vectors));
        }

        // Lamp
        //{
        //        Vector<N, T> lamp_org = center;
        //        for (unsigned i = 0; i < N - 2; ++i)
        //        {
        //                lamp_org -= (LAMP_SIZE / 2) * camera[i];
        //        }
        //        lamp_org += T(0.499) * camera[N - 2];
        //        lamp_org -= (LAMP_SIZE / 2) * camera[N - 1];
        //
        //        std::array<Vector<N, T>, N - 1> lamp_vectors;
        //        for (unsigned i = 0; i < N - 2; ++i)
        //        {
        //                lamp_vectors[i] = LAMP_SIZE * camera[i];
        //        }
        //        lamp_vectors[N - 2] = LAMP_SIZE * camera[N - 1];
        //
        //        std::unique_ptr<HyperplaneParallelotope<N, T, Color>> lamp =
        //                std::make_unique<HyperplaneParallelotope<N, T, Color>>(
        //                        METALNESS, ROUGHNESS, Color(color::rgb::WHITE), ALPHA, lamp_org, lamp_vectors);
        //        lamp->set_light_source(Color::illuminant(50, 50, 50));
        //
        //        shapes.push_back(std::move(lamp));
        //}

        if ((true))
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1.5;
                constexpr T FALLOFF_START = 80;
                constexpr T WIDTH = 90;

                Vector<N, T> lamp_org = center + T(0.49) * camera[N - 2];
                Vector<N, T> lamp_direction = -camera[N - 2];

                light_sources.push_back(std::make_unique<const SpotLight<N, T, Color>>(
                        lamp_org, lamp_direction, light, UNIT_INTENSITY_DISTANCE, FALLOFF_START, WIDTH));
        }
        else
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1;

                Vector<N, T> lamp_org = center + T(0.45) * camera[N - 2];

                light_sources.push_back(
                        std::make_unique<const PointLight<N, T, Color>>(lamp_org, light, UNIT_INTENSITY_DISTANCE));
        }

        std::unique_ptr<Projector<N, T>> projector;

        {
                const std::array<Vector<N, T>, N - 1> screen_axes = del_elem(camera, N - 1);
                const Vector<N, T> view_point = center - CAMERA * camera[N - 1];
                if ((true))
                {
                        projector = std::make_unique<PerspectiveProjector<N, T>>(
                                view_point, camera[N - 1], screen_axes, 70, screen_sizes);
                }
                else
                {
                        projector = std::make_unique<SphericalProjector<N, T>>(
                                view_point, camera[N - 1], screen_axes, 80, screen_sizes);
                }
        }

        return create_storage_scene<N, T>(
                background_light, std::move(projector), std::move(light_sources), std::move(shapes));
}
}

template <typename T, typename Color>
std::unique_ptr<const Scene<3, T, Color>> create_cornell_box_scene(
        const Color& light,
        const Color& background_light,
        int width,
        int height,
        std::unique_ptr<const Shape<3, T, Color>>&& shape,
        const Vector<3, T>& camera_direction,
        const Vector<3, T>& camera_up)
{
        namespace impl = cornell_box_scene_implementation;

        const geometry::BoundingBox bb = shape->bounding_box();
        const T size = (bb.max - bb.min).norm() * T(1.1);
        const Vector<3, T> center = (bb.max + bb.min) * T(0.5);

        const Vector<3, T> dir = size * camera_direction.normalized();
        const Vector<3, T> right = size * cross(camera_direction, camera_up).normalized();
        const Vector<3, T> up = size * cross(right, dir).normalized();

        return impl::create_cornell_box_scene(
                light, background_light, {width, height}, std::move(shape), {right, up, dir}, center);
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Scene<N, T, Color>> create_cornell_box_scene(
        const Color& light,
        const Color& background_light,
        int screen_size,
        std::unique_ptr<const Shape<N, T, Color>>&& shape)
{
        namespace impl = cornell_box_scene_implementation;

        const geometry::BoundingBox bb = shape->bounding_box();
        const T size = (bb.max - bb.min).norm() * T(1.1);
        const Vector<N, T> center = (bb.max + bb.min) * T(0.5);

        std::array<int, N - 1> screen_sizes;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                screen_sizes[i] = screen_size;
        }

        std::array<Vector<N, T>, N> camera;
        for (unsigned i = 0; i < N; ++i)
        {
                for (unsigned n = 0; n < i; ++n)
                {
                        camera[i][n] = 0;
                }
                camera[i][i] = size;
                for (unsigned n = i + 1; n < N; ++n)
                {
                        camera[i][n] = 0;
                }
        }
        camera[N - 1][N - 1] = -size;

        return impl::create_cornell_box_scene(light, background_light, screen_sizes, std::move(shape), camera, center);
}
}
