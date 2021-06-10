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

#include "painting.h"

#include "dimension.h"
#include "painter_scene.h"

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

// Максимальный размер экрана в пикселях для 3 измерений
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_3D = 10000;

// Размеры экрана в пикселях для 4 и более измерений
constexpr int PAINTER_MINIMUM_SCREEN_SIZE_ND = 50;
constexpr int PAINTER_MAXIMUM_SCREEN_SIZE_ND = 5000;
template <std::size_t N>
constexpr int PAINTER_DEFAULT_SCREEN_SIZE_ND = (N == 4) ? 500 : ((N == 5) ? 100 : PAINTER_MINIMUM_SCREEN_SIZE_ND);

using Precisions = std::tuple<double, float>;
constexpr std::size_t DEFAULT_PRECISION_INDEX = 0;

using Colors = std::tuple<color::Spectrum, color::Color>;
constexpr std::size_t DEFAULT_COLOR_INDEX = 0;

template <typename T, typename Color, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const view::info::Camera& camera,
        const color::Color& background_light,
        const double lighting_intensity,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        ProgressRatioList* progress_list)
{
        std::unique_ptr<const painter::Shape<N, T, Color>> shape = [&]
        {
                std::vector<const mesh::MeshObject<N>*> meshes;
                meshes.reserve(mesh_objects.size());
                for (const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object : mesh_objects)
                {
                        meshes.push_back(mesh_object.get());
                }
                ProgressRatio progress(progress_list);
                return painter::create_mesh<N, T, Color>(meshes, &progress);
        }();
        if (!shape)
        {
                MESSAGE_WARNING("No object to paint");
                return;
        }

        std::unique_ptr<const painter::Scene<N, T, Color>> scene;
        if constexpr (N == 3)
        {
                scene = create_painter_scene(
                        std::move(shape), to_vector<T>(camera.up), to_vector<T>(camera.forward),
                        to_vector<T>(camera.lighting), to_vector<T>(camera.view_center), camera.view_width,
                        dimension_parameters.width, dimension_parameters.height, parameters.cornell_box,
                        background_light, lighting_intensity);
        }
        else
        {
                scene = create_painter_scene(
                        std::move(shape), dimension_parameters.min_size, dimension_parameters.max_size,
                        parameters.cornell_box, background_light, lighting_intensity);
        }
        ASSERT(scene);

        std::string name = mesh_objects.size() != 1 ? "" : mesh_objects[0]->name();

        gui::painter_window::create_painter_window(
                name, parameters.thread_count, parameters.samples_per_pixel, !parameters.flat_facets, std::move(scene));
}

template <typename T, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const view::info::Camera& camera,
        const color::Color& background_light,
        const double lighting_intensity,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        ProgressRatioList* progress_list)
{
        static_assert(2 == std::tuple_size_v<Colors>);
        switch (parameters.color_index)
        {
        case 0:
                thread_function<T, std::tuple_element_t<0, Colors>>(
                        mesh_objects, camera, background_light, lighting_intensity, parameters, dimension_parameters,
                        progress_list);
                return;

        case 1:
                thread_function<T, std::tuple_element_t<1, Colors>>(
                        mesh_objects, camera, background_light, lighting_intensity, parameters, dimension_parameters,
                        progress_list);
                return;
        }
        error("Unknown color index " + to_string(parameters.color_index));
};

template <std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const view::info::Camera& camera,
        const color::Color& background_light,
        const double lighting_intensity,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        ProgressRatioList* progress_list)
{
        static_assert(2 == std::tuple_size_v<Precisions>);
        switch (parameters.precision_index)
        {
        case 0:
                thread_function<std::tuple_element_t<0, Precisions>>(
                        mesh_objects, camera, background_light, lighting_intensity, parameters, dimension_parameters,
                        progress_list);
                return;

        case 1:
                thread_function<std::tuple_element_t<1, Precisions>>(
                        mesh_objects, camera, background_light, lighting_intensity, parameters, dimension_parameters,
                        progress_list);
                return;
        }
        error("Unknown precision index " + to_string(parameters.precision_index));
};

template <std::size_t N>
bool has_facets(const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects)
{
        for (const std::shared_ptr<const mesh::MeshObject<N>>& object : mesh_objects)
        {
                if (!mesh::Reading<N>(*object).mesh().facets.empty())
                {
                        return true;
                }
        }
        return false;
}

template <std::size_t N>
std::function<void(ProgressRatioList*)> action_painter_function(
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const view::info::Camera& camera,
        const color::Color& background_light,
        const double lighting_intensity)
{
        if (!has_facets(mesh_objects))
        {
                MESSAGE_WARNING("No objects to paint");
                return nullptr;
        }

        static_assert(PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N> <= PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);
        static_assert(DEFAULT_PRECISION_INDEX < std::tuple_size_v<Precisions>);
        static_assert(DEFAULT_COLOR_INDEX < std::tuple_size_v<Colors>);

        static_assert(2 == std::tuple_size_v<Precisions>);
        const std::array<const char*, 2> precisions{
                type_bit_name<std::tuple_element_t<0, Precisions>>(),
                type_bit_name<std::tuple_element_t<1, Precisions>>()};

        static_assert(2 == std::tuple_size_v<Colors>);
        const std::array<const char*, 2> colors{
                std::tuple_element_t<0, Colors>::name(), std::tuple_element_t<1, Colors>::name()};

        if constexpr (N == 3)
        {
                std::optional<std::tuple<gui::dialog::PainterParameters, gui::dialog::PainterParameters3d>> parameters =
                        gui::dialog::PainterParameters3dDialog::show(
                                hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precisions,
                                DEFAULT_PRECISION_INDEX, colors, DEFAULT_COLOR_INDEX);

                if (!parameters)
                {
                        return nullptr;
                }

                return [=](ProgressRatioList* progress_list)
                {
                        thread_function(
                                mesh_objects, camera, background_light, lighting_intensity, std::get<0>(*parameters),
                                std::get<1>(*parameters), progress_list);
                };
        }
        else
        {
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> >= PAINTER_MINIMUM_SCREEN_SIZE_ND);
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> <= PAINTER_MAXIMUM_SCREEN_SIZE_ND);

                std::optional<std::tuple<gui::dialog::PainterParameters, gui::dialog::PainterParametersNd>> parameters =
                        gui::dialog::PainterParametersNdDialog::show(
                                N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND<N>,
                                PAINTER_MINIMUM_SCREEN_SIZE_ND, PAINTER_MAXIMUM_SCREEN_SIZE_ND,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precisions,
                                DEFAULT_PRECISION_INDEX, colors, DEFAULT_COLOR_INDEX);

                if (!parameters)
                {
                        return nullptr;
                }

                return [=](ProgressRatioList* progress_list)
                {
                        thread_function(
                                mesh_objects, camera, background_light, lighting_intensity, std::get<0>(*parameters),
                                std::get<1>(*parameters), progress_list);
                };
        }
}
}

std::function<void(ProgressRatioList*)> action_painter(
        const std::vector<storage::MeshObjectConst>& objects,
        const view::info::Camera& camera,
        const color::Color& background_light,
        const double lighting_intensity)
{
        std::set<std::size_t> dimensions;
        std::vector<storage::MeshObjectConst> visible_objects;
        for (const storage::MeshObjectConst& storage_object : objects)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& object)
                        {
                                if (object->visible())
                                {
                                        dimensions.insert(N);
                                        visible_objects.push_back(object);
                                }
                        },
                        storage_object);
        }
        if (visible_objects.empty())
        {
                MESSAGE_WARNING("No objects to paint");
                return nullptr;
        }
        if (dimensions.size() > 1)
        {
                MESSAGE_WARNING(
                        "Painting different dimensions is not supported, " + to_string(dimensions)
                        + " different dimensions.");
                return nullptr;
        }

        return apply_for_dimension(
                *dimensions.cbegin(),
                [&]<std::size_t N>(const Dimension<N>&)
                {
                        std::vector<std::shared_ptr<const mesh::MeshObject<N>>> meshes;
                        for (storage::MeshObjectConst& visible_object : visible_objects)
                        {
                                std::visit(
                                        [&]<std::size_t M>(std::shared_ptr<const mesh::MeshObject<M>>&& object)
                                        {
                                                if constexpr (N == M)
                                                {
                                                        meshes.push_back(std::move(object));
                                                }
                                        },
                                        std::move(visible_object));
                        }
                        return action_painter_function(meshes, camera, background_light, lighting_intensity);
                });
}
}
