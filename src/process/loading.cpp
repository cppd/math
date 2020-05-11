/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "loading.h"

#include "dimension.h"
#include "load.h"

#include <src/com/log.h>
#include <src/gui/dialogs/bound_cocone.h>
#include <src/gui/dialogs/file_dialog.h>
#include <src/gui/dialogs/object_selection.h>
#include <src/gui/dialogs/point_object.h>
#include <src/gui/dialogs/volume_object.h>
#include <src/model/mesh_utility.h>
#include <src/settings/utility.h>
#include <src/utility/file/sys.h>

namespace process
{
namespace
{
// Количество точек для готовых объектов.
constexpr int POINT_COUNT_MINIMUM = 100;
constexpr int POINT_COUNT_DEFAULT = 10000;
constexpr int POINT_COUNT_MAXIMUM = 1000000;

// Размер изображений по одному измерению для готовых объёмов.
constexpr int VOLUME_IMAGE_SIZE_MINIMUM = 10;
constexpr int VOLUME_IMAGE_SIZE_DEFAULT = 500;
constexpr int VOLUME_IMAGE_SIZE_MAXIMUM = 1000;

template <size_t N>
Vector<N, double> dimension_position(const vec3& position)
{
        if constexpr (N == 3)
        {
                return position;
        }
        else
        {
                return Vector<N, double>(0);
        }
}
}

std::function<void(ProgressRatioList*)> action_load_from_file(
        std::string file_name,
        bool use_object_selection_dialog,
        double object_size,
        const vec3& object_position,
        const std::function<void()>& clear_all)
{
        if (file_name.empty())
        {
                ASSERT(use_object_selection_dialog);

                std::string caption = "Open";
                bool read_only = true;

                std::vector<dialog::FileFilter> filters;
                for (const mesh::FileFormat& v : mesh::load_formats(settings::supported_dimensions()))
                {
                        dialog::FileFilter& f = filters.emplace_back();
                        f.name = v.format_name;
                        f.file_extensions = v.file_name_extensions;
                }

                if (!dialog::open_file(caption, filters, read_only, &file_name))
                {
                        return nullptr;
                }
        }

        std::unordered_set<dialog::ComputationType> objects_to_load;

        if (use_object_selection_dialog)
        {
                if (!dialog::object_selection(&objects_to_load))
                {
                        return nullptr;
                }
        }
        else
        {
                objects_to_load = dialog::object_selection_current();
        }

        double rho;
        double alpha;
        dialog::bound_cocone_parameters_current(&rho, &alpha);

        bool bound_cocone = objects_to_load.count(dialog::ComputationType::BoundCocone);
        bool cocone = objects_to_load.count(dialog::ComputationType::Cocone);
        bool convex_hull = objects_to_load.count(dialog::ComputationType::ConvexHull);
        bool mst = objects_to_load.count(dialog::ComputationType::Mst);

        return [=](ProgressRatioList* progress_list) {
                unsigned dimension = mesh::file_dimension(file_name);

                clear_all();

                apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                        std::shared_ptr<mesh::MeshObject<N>> mesh = load_from_file(
                                file_base_name(file_name), progress_list, file_name, object_size,
                                dimension_position<N>(object_position));

                        compute<N>(progress_list, convex_hull, cocone, bound_cocone, mst, *mesh, rho, alpha);
                });
        };
}

std::function<void(ProgressRatioList*)> action_load_from_mesh_repository(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        const std::function<void()>& clear_all)
{
        if (object_name.empty())
        {
                MESSAGE_ERROR("Empty mesh repository object name");
                return nullptr;
        }

        int point_count;

        if (!dialog::point_object_parameters(
                    dimension, object_name, POINT_COUNT_DEFAULT, POINT_COUNT_MINIMUM, POINT_COUNT_MAXIMUM,
                    &point_count))
        {
                return nullptr;
        }

        std::unordered_set<dialog::ComputationType> objects_to_load;

        if (!dialog::object_selection(&objects_to_load))
        {
                return nullptr;
        }

        double rho;
        double alpha;
        dialog::bound_cocone_parameters_current(&rho, &alpha);

        bool bound_cocone = objects_to_load.count(dialog::ComputationType::BoundCocone);
        bool cocone = objects_to_load.count(dialog::ComputationType::Cocone);
        bool convex_hull = objects_to_load.count(dialog::ComputationType::ConvexHull);
        bool mst = objects_to_load.count(dialog::ComputationType::Mst);

        return [=](ProgressRatioList* progress_list) {
                clear_all();

                apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                        std::shared_ptr<mesh::MeshObject<N>> mesh = load_from_mesh_repository(
                                object_name, object_size, dimension_position<N>(object_position), point_count,
                                *repository);

                        compute<N>(progress_list, convex_hull, cocone, bound_cocone, mst, *mesh, rho, alpha);
                });
        };
}

std::function<void(ProgressRatioList*)> action_load_from_volume_repository(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position)
{
        if (object_name.empty())
        {
                MESSAGE_ERROR("Empty volume repository object name");
                return nullptr;
        }

        int image_size;

        if (!dialog::volume_object_parameters(
                    dimension, object_name, VOLUME_IMAGE_SIZE_DEFAULT, VOLUME_IMAGE_SIZE_MINIMUM,
                    VOLUME_IMAGE_SIZE_MAXIMUM, &image_size))
        {
                return nullptr;
        }

        return [=](ProgressRatioList* /*progress_list*/) {
                apply_for_dimension(dimension, [&]<size_t N>(const Dimension<N>&) {
                        load_from_volume_repository(
                                object_name, object_size, dimension_position<N>(object_position), image_size,
                                *repository);
                });
        };
}
}
