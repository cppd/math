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

using DefaultFloatingPointType = double;

template <std::size_t N, typename T>
void thread_function(
        ProgressRatioList* progress_list,
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>& mesh_objects,
        const PainterSceneInfo<N, T>& scene_info,
        const Color& background_light,
        const Color::DataType& lighting_intensity,
        int thread_count,
        int samples_per_pixel,
        bool flat_facets)
{
        std::unique_ptr<const painter::Shape<N, T>> shape;
        {
                std::vector<const mesh::MeshObject<N>*> meshes;
                meshes.reserve(mesh_objects.size());
                for (const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object : mesh_objects)
                {
                        meshes.push_back(mesh_object.get());
                }
                ProgressRatio progress(progress_list);
                shape = painter::create_mesh<N, T>(meshes, &progress);
        }

        if (!shape)
        {
                MESSAGE_WARNING("No object to paint");
                return;
        }

        std::string name = mesh_objects.size() != 1 ? "" : mesh_objects[0]->name();

        std::unique_ptr<const painter::Scene<N, T>> scene =
                create_painter_scene(std::move(shape), scene_info, background_light, lighting_intensity);

        gui::painter_window::create_painter_window(
                name, thread_count, samples_per_pixel, !flat_facets, std::move(scene));
}

template <std::size_t N>
struct PainterData
{
        const std::vector<std::shared_ptr<const mesh::MeshObject<N>>>* mesh_objects;
        const view::info::Camera* camera;
        const Color* background_light;
        const Color::DataType* lighting_intensity;
};

template <typename T>
void thread_function(
        ProgressRatioList* progress_list,
        const PainterData<3>& data,
        const gui::dialog::PainterParameters3d& parameters)
{
        PainterSceneInfo<3, T> scene_info(
                data.camera->up, data.camera->forward, data.camera->lighting, data.camera->view_center,
                data.camera->view_width, parameters.width, parameters.height, parameters.cornell_box);

        thread_function<3, T>(
                progress_list, *data.mesh_objects, scene_info, *data.background_light, *data.lighting_intensity,
                parameters.thread_count, parameters.samples_per_pixel, parameters.flat_facets);
}

template <typename T, std::size_t N>
void thread_function(
        ProgressRatioList* progress_list,
        const PainterData<N>& data,
        const gui::dialog::PainterParametersNd& parameters)
{
        PainterSceneInfo<N, T> scene_info(parameters.min_size, parameters.max_size, parameters.cornell_box);

        thread_function<N, T>(
                progress_list, *data.mesh_objects, scene_info, *data.background_light, *data.lighting_intensity,
                parameters.thread_count, parameters.samples_per_pixel, parameters.flat_facets);
}

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
        const Color& background_light,
        const Color::DataType& lighting_intensity)
{
        ASSERT(!mesh_objects.empty());

        if (!has_facets(mesh_objects))
        {
                MESSAGE_WARNING("No objects to paint");
                return nullptr;
        }

        static_assert(PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N> <= PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);

        static_assert(
                std::is_same_v<float, DefaultFloatingPointType> || std::is_same_v<double, DefaultFloatingPointType>);
        const std::array<const char*, 2> precisions{type_bit_name<double>(), type_bit_name<float>()};
        const int default_precision_index = std::is_same_v<double, DefaultFloatingPointType> ? 0 : 1;

        if constexpr (N == 3)
        {
                std::optional<gui::dialog::PainterParameters3d> parameters =
                        gui::dialog::PainterParameters3dDialog::show(
                                hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precisions,
                                default_precision_index);

                if (!parameters)
                {
                        return nullptr;
                }

                ASSERT(parameters->precision_index == 0 || parameters->precision_index == 1);

                return [=](ProgressRatioList* progress_list)
                {
                        PainterData<N> data;
                        data.mesh_objects = &mesh_objects;
                        data.camera = &camera;
                        data.background_light = &background_light;
                        data.lighting_intensity = &lighting_intensity;

                        if (parameters->precision_index == 0)
                        {
                                thread_function<double>(progress_list, data, *parameters);
                        }
                        else
                        {
                                thread_function<float>(progress_list, data, *parameters);
                        }
                };
        }
        else
        {
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> >= PAINTER_MINIMUM_SCREEN_SIZE_ND);
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> <= PAINTER_MAXIMUM_SCREEN_SIZE_ND);

                std::optional<gui::dialog::PainterParametersNd> parameters =
                        gui::dialog::PainterParametersNdDialog::show(
                                N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND<N>,
                                PAINTER_MINIMUM_SCREEN_SIZE_ND, PAINTER_MAXIMUM_SCREEN_SIZE_ND,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>, precisions,
                                default_precision_index);

                if (!parameters)
                {
                        return nullptr;
                }

                ASSERT(parameters->precision_index == 0 || parameters->precision_index == 1);

                return [=](ProgressRatioList* progress_list)
                {
                        PainterData<N> data;
                        data.mesh_objects = &mesh_objects;
                        data.camera = &camera;
                        data.background_light = &background_light;
                        data.lighting_intensity = &lighting_intensity;

                        if (parameters->precision_index == 0)
                        {
                                thread_function<double>(progress_list, data, *parameters);
                        }
                        else
                        {
                                thread_function<float>(progress_list, data, *parameters);
                        }
                };
        }
}
}

std::function<void(ProgressRatioList*)> action_painter(
        const std::vector<storage::MeshObjectConst>& objects,
        const view::info::Camera& camera,
        const Color& background_light,
        const Color::DataType& lighting_intensity)
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
