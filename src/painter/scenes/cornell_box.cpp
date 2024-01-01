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

#include "cornell_box.h"

#include "storage.h"

#include "../lights/parallelotope_light.h"
#include "../lights/point_light.h"
#include "../lights/spot_light.h"
#include "../objects.h"
#include "../projectors/perspective_projector.h"
#include "../projectors/spherical_projector.h"
#include "../shapes/hyperplane_parallelotope.h"
#include "../shapes/parallelotope.h"

#include <src/color/colors.h>
#include <src/com/arrays.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace ns::painter::scenes
{
namespace
{
enum class ProjectorType
{
        PERSPECTIVE,
        SPHERICAL
};

enum class LightType
{
        PARALLELOTOPE,
        SPOTLIGHT,
        POINT
};

constexpr ProjectorType PROJECTOR_TYPE = ProjectorType::PERSPECTIVE;
constexpr LightType LIGHT_TYPE = LightType::PARALLELOTOPE;

template <typename Color, std::size_t N, typename T>
std::vector<std::unique_ptr<const Shape<N, T, Color>>> create_shapes(
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center)
{
        constexpr T BOX_SIZE = 0.16;
        constexpr T BOX_SPACE = 0.06;
        constexpr T NEAR = 0.9;
        constexpr T DEPTH = NEAR + 0.5 + BOX_SIZE + 2 * BOX_SPACE;

        constexpr T ALPHA = 1;
        constexpr T METALNESS = 0;
        constexpr T ROUGHNESS = 0.15;

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

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes;

        // Walls
        {
                std::array<Vector<N, T>, N> walls_vectors = camera;
                walls_vectors[N - 1] *= DEPTH;

                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::RED), ALPHA, org,
                                del_elem(walls_vectors, i)));
                        shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<N, T, Color>>(
                                METALNESS, ROUGHNESS, Color((i >= 1) ? color::rgb::WHITE : color::rgb::GREEN), ALPHA,
                                org + walls_vectors[i], del_elem(walls_vectors, i)));
                }
                shapes.push_back(std::make_unique<shapes::HyperplaneParallelotope<N, T, Color>>(
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

                shapes.push_back(std::make_unique<shapes::Parallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::MAGENTA), ALPHA, box_org, box_vectors));
        }

        return shapes;
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

        switch (PROJECTOR_TYPE)
        {
        case ProjectorType::PERSPECTIVE:
        {
                return std::make_unique<projectors::PerspectiveProjector<N, T>>(
                        view_point, camera[N - 1], screen_axes, 60, screen_size);
        }
        case ProjectorType::SPHERICAL:
        {
                return std::make_unique<projectors::SphericalProjector<N, T>>(
                        view_point, camera[N - 1], screen_axes, 70, screen_size);
        }
        }
        error_fatal("Unknown projector type " + to_string(enum_to_int(PROJECTOR_TYPE)));
}

template <std::size_t N, typename T, typename Color>
void create_light_sources(
        const Color& light,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center,
        std::vector<std::unique_ptr<LightSource<N, T, Color>>>* const lights,
        std::vector<std::unique_ptr<const Shape<N, T, Color>>>* const shapes)
{
        constexpr T FALLOFF_START = 60;
        constexpr T WIDTH = 72;

        switch (LIGHT_TYPE)
        {
        case LightType::PARALLELOTOPE:
        {
                constexpr T SIZE = 0.1;
                constexpr T INTENSITY = power<N - 1>(T{8});

                constexpr T ALPHA = 1;
                constexpr T METALNESS = 0;
                constexpr T ROUGHNESS = 1;

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

                const Vector<N, T> direction = -camera[N - 2];

                auto shape = std::make_unique<shapes::HyperplaneParallelotope<N, T, Color>>(
                        METALNESS, ROUGHNESS, Color(color::rgb::WHITE), ALPHA, org, vectors);

                lights->push_back(std::make_unique<lights::ParallelotopeLight<N, T, Color>>(
                        shape->hyperplane_parallelotope(), direction, INTENSITY * light, FALLOFF_START, WIDTH));

                shape->set_light_source(lights->back().get());

                shapes->push_back(std::move(shape));

                return;
        }
        case LightType::SPOTLIGHT:
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1.5;

                const Vector<N, T> org = center + T{0.49} * camera[N - 2];
                const Vector<N, T> direction = -camera[N - 2];

                lights->push_back(std::make_unique<lights::SpotLight<N, T, Color>>(
                        org, direction, light, UNIT_INTENSITY_DISTANCE, FALLOFF_START, WIDTH));

                return;
        }
        case LightType::POINT:
        {
                constexpr T UNIT_INTENSITY_DISTANCE = 1;

                const Vector<N, T> org = center + T{0.45} * camera[N - 2];

                lights->push_back(
                        std::make_unique<lights::PointLight<N, T, Color>>(org, light, UNIT_INTENSITY_DISTANCE));

                return;
        }
        }
        error_fatal("Unknown light type " + to_string(enum_to_int(LIGHT_TYPE)));
}

template <std::size_t N, typename T, typename Color>
StorageScene<N, T, Color> create_cornell_box_scene(
        const Color& light,
        const Color& /*background_light*/,
        const std::array<int, N - 1>& screen_size,
        const std::array<Vector<N, T>, N>& camera,
        const Vector<N, T>& center,
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        progress::Ratio* const progress)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> shapes = create_shapes<Color>(camera, center);

        shapes.push_back(std::move(shape));

        std::vector<std::unique_ptr<LightSource<N, T, Color>>> light_sources;
        create_light_sources(light, camera, center, &light_sources, &shapes);

        std::unique_ptr<Projector<N, T>> projector = create_projector(screen_size, camera, center);

        return create_storage_scene<N, T>(
                /*background_light*/ Color{0}, /*clip_plane_equation*/ std::nullopt, std::move(projector),
                std::move(light_sources), std::move(shapes), progress);
}

template <std::size_t N, typename T>
std::tuple<std::array<Vector<N, T>, N>, Vector<N, T>> camera_and_center(const geometry::spatial::BoundingBox<N, T>& bb)
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
StorageScene<N, T, Color> create_cornell_box_scene(
        std::unique_ptr<const Shape<N, T, Color>>&& shape,
        const Color& light,
        const Color& background_light,
        const std::array<int, N - 1>& screen_size,
        progress::Ratio* const progress)
{
        static_assert(N >= 3);

        const auto [camera, center] = camera_and_center(shape->bounding_box());

        return create_cornell_box_scene(
                light, background_light, screen_size, camera, center, std::move(shape), progress);
}

#define TEMPLATE(N, T, C)                                                                                     \
        template StorageScene<(N), T, C> create_cornell_box_scene(                                            \
                std::unique_ptr<const Shape<(N), T, C>>&&, const C&, const C&, const std::array<int, (N)-1>&, \
                progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
