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
#include <src/gui/dialogs/painter_3d.h>
#include <src/gui/dialogs/painter_nd.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/settings/painter.h>

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

        if constexpr (N == 3)
        {
                std::optional<gui::dialog::Painter3dParameters> parameters =
                        gui::dialog::Painter3dParametersDialog::show(
                                hardware_concurrency(), camera.width, camera.height, PAINTER_MAXIMUM_SCREEN_SIZE_3D,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);

                if (!parameters)
                {
                        return nullptr;
                }

                return [=](ProgressRatioList* progress_list)
                {
                        auto f = [&]<typename T>(T)
                        {
                                PainterSceneInfo<N, T> scene_info(
                                        camera.up, camera.forward, camera.lighting, camera.view_center,
                                        camera.view_width, parameters->width, parameters->height,
                                        parameters->cornell_box);

                                thread_function<N, T>(
                                        progress_list, mesh_objects, scene_info, background_light, lighting_intensity,
                                        parameters->thread_count, parameters->samples_per_pixel,
                                        parameters->flat_facets);
                        };
                        f(settings::painter::FloatingPoint());
                };
        }
        else
        {
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> >= PAINTER_MINIMUM_SCREEN_SIZE_ND);
                static_assert(PAINTER_DEFAULT_SCREEN_SIZE_ND<N> <= PAINTER_MAXIMUM_SCREEN_SIZE_ND);

                std::optional<gui::dialog::PainterNdParameters> parameters =
                        gui::dialog::PainterNdParametersDialog::show(
                                N, hardware_concurrency(), PAINTER_DEFAULT_SCREEN_SIZE_ND<N>,
                                PAINTER_MINIMUM_SCREEN_SIZE_ND, PAINTER_MAXIMUM_SCREEN_SIZE_ND,
                                PAINTER_DEFAULT_SAMPLES_PER_PIXEL<N>, PAINTER_MAXIMUM_SAMPLES_PER_PIXEL<N>);

                if (!parameters)
                {
                        return nullptr;
                }

                return [=](ProgressRatioList* progress_list)
                {
                        auto f = [&]<typename T>(T)
                        {
                                PainterSceneInfo<N, T> scene_info(
                                        parameters->min_size, parameters->max_size, parameters->cornell_box);

                                thread_function<N, T>(
                                        progress_list, mesh_objects, scene_info, background_light, lighting_intensity,
                                        parameters->thread_count, parameters->samples_per_pixel,
                                        parameters->flat_facets);
                        };
                        f(settings::painter::FloatingPoint());
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
                MESSAGE_WARNING("Painting different dimensions is not supported: " + to_string(dimensions) + ".");
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
