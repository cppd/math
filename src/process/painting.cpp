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

#include "painting.h"

#include "dimension.h"
#include "painter_scene.h"

#include <src/color/illuminants.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/type/name.h>
#include <src/gui/dialogs/painter_parameters_3d.h>
#include <src/gui/dialogs/painter_parameters_nd.h>
#include <src/gui/painter_window/painter_window.h>

#include <set>
#include <tuple>

namespace ns::process
{
namespace
{
template <std::size_t N>
constexpr int PAINTER_DEFAULT_SAMPLES_PER_PIXEL = (N == 3) ? 25 : 1;
template <std::size_t N>
constexpr int PAINTER_MAXIMUM_SAMPLES_PER_PIXEL = power<N - 1>(10u);

constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_3D = 10000;

constexpr int PAINTER_MINIMUM_SCREEN_SIZE_ND = 50;
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_ND = 5000;
template <std::size_t N>
constexpr int PAINTER_DEFAULT_SCREEN_SIZE_ND = (N == 4) ? 300 : ((N == 5) ? 100 : PAINTER_MINIMUM_SCREEN_SIZE_ND);

using Precisions = std::tuple<double, float>;
constexpr std::size_t DEFAULT_PRECISION_INDEX = 0;
static_assert(DEFAULT_PRECISION_INDEX < std::tuple_size_v<Precisions>);

using Colors = std::tuple<color::Spectrum, color::Color>;
constexpr std::size_t DEFAULT_COLOR_INDEX = 0;
static_assert(DEFAULT_COLOR_INDEX < std::tuple_size_v<Colors>);

std::array<const char*, 2> precision_names()
{
        static_assert(2 == std::tuple_size_v<Precisions>);
        return {type_bit_name<std::tuple_element_t<0, Precisions>>(),
                type_bit_name<std::tuple_element_t<1, Precisions>>()};
}

std::array<const char*, 2> color_names()
{
        static_assert(2 == std::tuple_size_v<Colors>);
        return {std::tuple_element_t<0, Colors>::name(), std::tuple_element_t<1, Colors>::name()};
}

template <typename T, typename Color, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const Color& light,
        const Color& background_light,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        progress::RatioList* const progress_list)
{
        std::unique_ptr<const painter::Shape<N, T, Color>> shape = [&]
        {
                std::vector<const model::mesh::MeshObject<N>*> meshes;
                meshes.reserve(objects.size());
                for (const std::shared_ptr<const model::mesh::MeshObject<N>>& object : objects)
                {
                        meshes.push_back(object.get());
                }
                progress::Ratio progress(progress_list);
                constexpr bool WRITE_LOG = true;
                return painter::create_mesh<N, T, Color>(meshes, WRITE_LOG, &progress);
        }();
        if (!shape)
        {
                message_warning("No object to paint");
                return;
        }

        std::unique_ptr<const painter::Scene<N, T, Color>> scene;
        if constexpr (N == 3)
        {
                progress::Ratio progress(nullptr);
                scene = create_painter_scene(
                        std::move(shape), to_vector<T>(camera.up), to_vector<T>(camera.forward),
                        to_vector<T>(camera.lighting), to_vector<T>(camera.view_center), camera.view_width,
                        clip_plane.equation ? std::optional(to_vector<T>(*clip_plane.equation)) : std::nullopt,
                        dimension_parameters.width, dimension_parameters.height, parameters.cornell_box, light,
                        background_light, &progress);
        }
        else
        {
                progress::Ratio progress(nullptr);
                scene = create_painter_scene(
                        std::move(shape), dimension_parameters.max_size, parameters.cornell_box, light,
                        background_light, &progress);
        }
        ASSERT(scene);

        const std::string name = objects.size() != 1 ? "" : objects[0]->name();

        gui::painter_window::create_painter_window(
                name, parameters.thread_count, parameters.samples_per_pixel, !parameters.flat_facets, std::move(scene));
}

template <typename T, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        progress::RatioList* const progress_list)
{
        static_assert(2 == std::tuple_size_v<Colors>);
        switch (parameters.color_index)
        {
        case 0:
        {
                using Color = std::tuple_element_t<0, Colors>;
                thread_function<T, Color>(
                        objects, camera, clip_plane, std::get<Color>(lighting_color),
                        to_illuminant<Color>(background_color), parameters, dimension_parameters, progress_list);
                return;
        }
        case 1:
        {
                using Color = std::tuple_element_t<1, Colors>;
                thread_function<T, Color>(
                        objects, camera, clip_plane, std::get<Color>(lighting_color),
                        to_illuminant<Color>(background_color), parameters, dimension_parameters, progress_list);
                return;
        }
        }
        error("Unknown color index " + to_string(parameters.color_index));
}

template <std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        progress::RatioList* const progress_list)
{
        static_assert(2 == std::tuple_size_v<Precisions>);
        switch (parameters.precision_index)
        {
        case 0:
                thread_function<std::tuple_element_t<0, Precisions>>(
                        objects, camera, clip_plane, lighting_color, background_color, parameters, dimension_parameters,
                        progress_list);
                return;

        case 1:
                thread_function<std::tuple_element_t<1, Precisions>>(
                        objects, camera, clip_plane, lighting_color, background_color, parameters, dimension_parameters,
                        progress_list);
                return;
        }
        error("Unknown precision index " + to_string(parameters.precision_index));
}

template <std::size_t N>
        requires(N == 3)
std::function<void(progress::RatioList*)> action_painter_function(
        std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>&& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color)
{
        static_assert(PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N> <= PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);

        std::optional<std::tuple<gui::dialog::PainterParameters, gui::dialog::PainterParameters3d>> parameters =
                gui::dialog::PainterParameters3dDialog::show(
                        hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                        PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precision_names(),
                        DEFAULT_PRECISION_INDEX, color_names(), DEFAULT_COLOR_INDEX);

        if (!parameters)
        {
                return nullptr;
        }

        std::shared_ptr shared_objects =
                std::make_shared<std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>>(std::move(objects));

        return [=, shared_objects = std::move(shared_objects)](progress::RatioList* const progress_list)
        {
                thread_function(
                        *shared_objects, camera, clip_plane, lighting_color, background_color, std::get<0>(*parameters),
                        std::get<1>(*parameters), progress_list);
        };
}

template <std::size_t N>
        requires(N >= 4)
std::function<void(progress::RatioList*)> action_painter_function(
        std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>&& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color)
{
        static_assert(PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N> <= PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);
        static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> >= PAINTER_MINIMUM_SCREEN_SIZE_ND);
        static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> <= PAINTER_MAXIMUM_SCREEN_SIZE_ND);

        std::optional<std::tuple<gui::dialog::PainterParameters, gui::dialog::PainterParametersNd>> parameters =
                gui::dialog::PainterParametersNdDialog::show(
                        N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND<N>, PAINTER_MINIMUM_SCREEN_SIZE_ND,
                        PAINTER_MAXIMUM_SCREEN_SIZE_ND, PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>,
                        PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precision_names(), DEFAULT_PRECISION_INDEX, color_names(),
                        DEFAULT_COLOR_INDEX);

        if (!parameters)
        {
                return nullptr;
        }

        std::shared_ptr shared_objects =
                std::make_shared<std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>>(std::move(objects));

        return [=, shared_objects = std::move(shared_objects)](progress::RatioList* const progress_list)
        {
                thread_function(
                        *shared_objects, camera, clip_plane, lighting_color, background_color, std::get<0>(*parameters),
                        std::get<1>(*parameters), progress_list);
        };
}

std::tuple<std::vector<storage::MeshObjectConst>, std::size_t> copy_paint_objects(
        const std::vector<storage::MeshObjectConst>& objects)
{
        std::tuple<std::vector<storage::MeshObjectConst>, std::size_t> res;

        std::set<std::size_t> dimensions;
        for (const storage::MeshObjectConst& storage_object : objects)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<const model::mesh::MeshObject<N>>& object)
                        {
                                model::mesh::Reading reading(*object);
                                if (reading.visible() && !reading.mesh().facets.empty())
                                {
                                        dimensions.insert(N);
                                        std::get<0>(res).push_back(object);
                                }
                        },
                        storage_object);

                if (dimensions.size() > 1)
                {
                        message_warning(
                                "Painting different dimensions is not supported, dimensions: " + to_string(dimensions));
                        return {};
                }
        }

        if (std::get<0>(res).empty())
        {
                message_warning("No objects to paint");
                return {};
        }

        std::get<1>(res) = *dimensions.cbegin();

        return res;
}
}

std::function<void(progress::RatioList*)> action_painter(
        const std::vector<storage::MeshObjectConst>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color)
{
        std::vector<storage::MeshObjectConst> paint_objects;
        std::size_t dimension = 0;

        std::tie(paint_objects, dimension) = copy_paint_objects(objects);

        if (paint_objects.empty())
        {
                return nullptr;
        }

        ASSERT(dimension > 0);

        const auto f = [&]<std::size_t N>(const Dimension<N>&)
        {
                std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>> meshes;
                for (storage::MeshObjectConst& paint_object : paint_objects)
                {
                        std::visit(
                                [&meshes]<std::size_t M>(std::shared_ptr<const model::mesh::MeshObject<M>>&& object)
                                {
                                        if constexpr (N == M)
                                        {
                                                meshes.push_back(std::move(object));
                                        }
                                },
                                std::move(paint_object));
                }
                return action_painter_function(std::move(meshes), camera, clip_plane, lighting_color, background_color);
        };

        return apply_for_dimension(dimension, f);
}
}
