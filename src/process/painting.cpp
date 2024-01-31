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

#include "painting.h"

#include "dimension.h"

#include <src/color/color.h>
#include <src/com/arrays.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/message.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/type/name.h>
#include <src/gui/dialogs/painter_parameters.h>
#include <src/gui/dialogs/painter_parameters_3d.h>
#include <src/gui/dialogs/painter_parameters_nd.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/model/mesh_object.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/painter/painter.h>
#include <src/painter/scenes/cornell_box.h>
#include <src/painter/scenes/simple.h>
#include <src/painter/scenes/storage.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress.h>
#include <src/progress/progress_list.h>
#include <src/storage/types.h>
#include <src/view/event.h>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ns::process
{
namespace
{
template <std::size_t N>
constexpr int SAMPLES_PER_PIXEL = (N == 3) ? 25 : 1;
template <std::size_t N>
constexpr int SAMPLES_PER_PIXEL_MAXIMUM = power<N - 1>(10u);

constexpr int SCREEN_SIZE_3D_MAXIMUM = 10000;

constexpr int SCREEN_SIZE_ND_MINIMUM = 50;
constexpr int SCREEN_SIZE_ND_MAXIMUM = 5000;
template <std::size_t N>
constexpr int SCREEN_SIZE_ND = (N == 4) ? 300 : ((N == 5) ? 100 : SCREEN_SIZE_ND_MINIMUM);

using Precisions = std::tuple<double, float>;
constexpr std::size_t PRECISION_INDEX = 0;
static_assert(PRECISION_INDEX < std::tuple_size_v<Precisions>);

using Colors = std::tuple<color::Spectrum, color::Color>;
template <std::size_t N>
constexpr std::size_t COLOR_INDEX = (N == 3) ? 0 : 1;

constexpr painter::Integrator INTEGRATOR = painter::Integrator::PT;

template <typename T>
std::shared_ptr<std::vector<T>> move_to_shared_ptr(std::vector<T>&& v)
{
        return std::make_shared<std::vector<T>>(std::move(v));
}

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

std::size_t integrator_to_index(const painter::Integrator integrator)
{
        switch (integrator)
        {
        case painter::Integrator::PT:
                return 0;
        case painter::Integrator::BPT:
                return 1;
        }
        error("Unknown integrator " + to_string(enum_to_int(integrator)));
}

painter::Integrator index_to_integrator(const std::size_t index)
{
        switch (index)
        {
        case 0:
                return painter::Integrator::PT;
        case 1:
                return painter::Integrator::BPT;
        default:
                error("Unknown integrator index " + to_string(index));
        }
}

std::array<const char*, 2> integrator_names()
{
        return {"PT", "BPT"};
}

template <std::size_t N, typename T>
std::optional<numerical::Vector<N + 1, T>> make_clip_plane_equation(const view::info::ClipPlane& clip_plane)
{
        if constexpr (N >= 4)
        {
                return std::nullopt;
        }
        else
        {
                if (clip_plane.equation)
                {
                        return to_vector<T>(*clip_plane.equation);
                }
                return std::nullopt;
        }
}

template <typename T>
std::optional<T> make_clip_plane_position(const view::info::ClipPlane& clip_plane)
{
        if (clip_plane.position)
        {
                return *clip_plane.position;
        }
        return std::nullopt;
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const painter::Shape<N, T, Color>> make_shape(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        progress::RatioList* const progress_list)
{
        constexpr bool WRITE_LOG = true;

        std::vector<const model::mesh::MeshObject<N>*> meshes;
        meshes.reserve(objects.size());

        for (const std::shared_ptr<const model::mesh::MeshObject<N>>& object : objects)
        {
                meshes.push_back(object.get());
        }

        progress::Ratio progress(progress_list);

        return painter::shapes::create_mesh<N, T, Color>(meshes, clip_plane_equation, WRITE_LOG, &progress);
}

template <std::size_t N, typename T, typename Color>
        requires (N == 3)
painter::scenes::StorageScene<N, T, Color> make_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>> shape,
        const view::info::Camera& camera,
        const view::info::ClipPlane& /*clip_plane*/,
        const std::type_identity_t<T> front_light_proportion,
        const Color& light,
        const Color& background_light,
        const gui::dialog::PainterParameters& parameters,
        const gui::dialog::PainterParameters3d& dimension_parameters,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation)
{
        progress::Ratio progress(nullptr);

        if (parameters.cornell_box)
        {
                return painter::scenes::create_cornell_box_scene(
                        std::move(shape), light, background_light,
                        {dimension_parameters.width, dimension_parameters.height}, &progress);
        }

        return painter::scenes::create_simple_scene(
                std::move(shape), light, background_light, clip_plane_equation, front_light_proportion,
                dimension_parameters.width, dimension_parameters.height, to_vector<T>(camera.up),
                to_vector<T>(camera.forward), to_vector<T>(camera.lighting), to_vector<T>(camera.view_center),
                camera.view_width, &progress);
}

template <std::size_t N, typename T, typename Color>
        requires (N >= 4)
painter::scenes::StorageScene<N, T, Color> make_scene(
        std::unique_ptr<const painter::Shape<N, T, Color>> shape,
        const view::info::Camera& /*camera*/,
        const view::info::ClipPlane& clip_plane,
        const std::type_identity_t<T> front_light_proportion,
        const Color& light,
        const Color& background_light,
        const gui::dialog::PainterParameters& parameters,
        const gui::dialog::PainterParametersNd& dimension_parameters,
        const std::optional<numerical::Vector<N + 1, T>>& /*clip_plane_equation*/)
{
        progress::Ratio progress(nullptr);

        if (parameters.cornell_box)
        {
                return painter::scenes::create_cornell_box_scene(
                        std::move(shape), light, background_light,
                        make_array_value<int, N - 1>(dimension_parameters.max_size), &progress);
        }

        return painter::scenes::create_simple_scene(
                std::move(shape), light, background_light, make_clip_plane_position<T>(clip_plane),
                front_light_proportion, dimension_parameters.max_size, &progress);
}

template <typename T, typename Color, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::type_identity_t<T> front_light_proportion,
        const Color& light,
        const Color& background_light,
        const gui::dialog::PainterParameters& parameters,
        const Parameters& dimension_parameters,
        progress::RatioList* const progress_list)
{
        const auto clip_plane_equation = make_clip_plane_equation<N, T>(clip_plane);

        std::unique_ptr<const painter::Shape<N, T, Color>> shape =
                make_shape<N, T, Color>(objects, clip_plane_equation, progress_list);

        if (!shape)
        {
                message_warning("No object to paint");
                return;
        }

        painter::scenes::StorageScene<N, T, Color> scene = make_scene(
                std::move(shape), camera, clip_plane, front_light_proportion, light, background_light, parameters,
                dimension_parameters, clip_plane_equation);

        const std::string name = objects.size() != 1 ? "" : objects[0]->name();

        gui::painter_window::create_painter_window(
                name, index_to_integrator(parameters.integrator_index), parameters.thread_count,
                parameters.samples_per_pixel, parameters.flat_shading, std::move(scene));
}

template <typename T, std::size_t N, typename Parameters>
void thread_function(
        const std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const std::type_identity_t<T> front_light_proportion,
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
                        objects, camera, clip_plane, front_light_proportion, std::get<Color>(lighting_color),
                        to_illuminant<Color>(background_color), parameters, dimension_parameters, progress_list);
                return;
        }
        case 1:
        {
                using Color = std::tuple_element_t<1, Colors>;
                thread_function<T, Color>(
                        objects, camera, clip_plane, front_light_proportion, std::get<Color>(lighting_color),
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
        const double front_light_proportion,
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
                        objects, camera, clip_plane, front_light_proportion, lighting_color, background_color,
                        parameters, dimension_parameters, progress_list);
                return;

        case 1:
                thread_function<std::tuple_element_t<1, Precisions>>(
                        objects, camera, clip_plane, front_light_proportion, lighting_color, background_color,
                        parameters, dimension_parameters, progress_list);
                return;
        }
        error("Unknown precision index " + to_string(parameters.precision_index));
}

template <std::size_t N>
        requires (N == 3)
std::function<void(progress::RatioList*)> action_painter_function(
        std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>&& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const double front_light_proportion,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color)
{
        static_assert(SAMPLES_PER_PIXEL<N> <= SAMPLES_PER_PIXEL_MAXIMUM<N>);
        static_assert(COLOR_INDEX<N> < std::tuple_size_v<Colors>);

        const auto parameters = gui::dialog::PainterParameters3dDialog::show(
                hardware_concurrency(), camera.width, camera.height, SCREEN_SIZE_3D_MAXIMUM, SAMPLES_PER_PIXEL<N>,
                SAMPLES_PER_PIXEL_MAXIMUM<N>, precision_names(), PRECISION_INDEX, color_names(), COLOR_INDEX<N>,
                integrator_names(), integrator_to_index(INTEGRATOR));

        if (!parameters)
        {
                return nullptr;
        }

        return [=, objects = move_to_shared_ptr(std::move(objects))](progress::RatioList* const progress_list)
        {
                thread_function(
                        *objects, camera, clip_plane, front_light_proportion, lighting_color, background_color,
                        std::get<0>(*parameters), std::get<1>(*parameters), progress_list);
        };
}

template <std::size_t N>
        requires (N >= 4)
std::function<void(progress::RatioList*)> action_painter_function(
        std::vector<std::shared_ptr<const model::mesh::MeshObject<N>>>&& objects,
        const view::info::Camera& camera,
        const view::info::ClipPlane& clip_plane,
        const double front_light_proportion,
        const std::tuple<color::Spectrum, color::Color>& lighting_color,
        const color::Color& background_color)
{
        static_assert(SAMPLES_PER_PIXEL<N> <= SAMPLES_PER_PIXEL_MAXIMUM<N>);
        static_assert(SCREEN_SIZE_ND<N> >= SCREEN_SIZE_ND_MINIMUM);
        static_assert(SCREEN_SIZE_ND<N> <= SCREEN_SIZE_ND_MAXIMUM);
        static_assert(COLOR_INDEX<N> < std::tuple_size_v<Colors>);

        const auto parameters = gui::dialog::PainterParametersNdDialog::show(
                N, hardware_concurrency(), SCREEN_SIZE_ND<N>, SCREEN_SIZE_ND_MINIMUM, SCREEN_SIZE_ND_MAXIMUM,
                SAMPLES_PER_PIXEL<N>, SAMPLES_PER_PIXEL_MAXIMUM<N>, precision_names(), PRECISION_INDEX, color_names(),
                COLOR_INDEX<N>, integrator_names(), integrator_to_index(INTEGRATOR));

        if (!parameters)
        {
                return nullptr;
        }

        return [=, objects = move_to_shared_ptr(std::move(objects))](progress::RatioList* const progress_list)
        {
                thread_function(
                        *objects, camera, clip_plane, front_light_proportion, lighting_color, background_color,
                        std::get<0>(*parameters), std::get<1>(*parameters), progress_list);
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
                                const model::mesh::Reading reading(*object);
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
        const double front_light_proportion,
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
                return action_painter_function(
                        std::move(meshes), camera, clip_plane, front_light_proportion, lighting_color,
                        background_color);
        };

        return apply_for_dimension(dimension, f);
}
}
