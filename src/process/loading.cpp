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

#include <src/com/message.h>
#include <src/gui/dialogs/bound_cocone.h>
#include <src/gui/dialogs/file_dialog.h>
#include <src/gui/dialogs/object_selection.h>
#include <src/gui/dialogs/point_object.h>
#include <src/gui/dialogs/volume_object.h>
#include <src/model/mesh_utility.h>
#include <src/settings/utility.h>
#include <src/utility/file/path.h>

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
}

std::function<void(ProgressRatioList*)> action_load_mesh_from_file(
        std::filesystem::path file_name,
        bool use_object_selection_dialog)
{
        if (file_name.empty())
        {
                ASSERT(use_object_selection_dialog);

                std::string caption = "Open";
                bool read_only = true;

                std::vector<gui::dialog::FileFilter> filters;
                for (const mesh::FileFormat& v : mesh::load_formats(settings::supported_dimensions()))
                {
                        gui::dialog::FileFilter& f = filters.emplace_back();
                        f.name = v.format_name;
                        f.file_extensions = v.file_name_extensions;
                }

                std::optional<std::string> file_name_string = gui::dialog::open_file(caption, filters, read_only);
                if (!file_name_string)
                {
                        return nullptr;
                }
                file_name = path_from_utf8(*file_name_string);
        }

        gui::dialog::ObjectSelectionParameters selection_parameters;
        if (use_object_selection_dialog)
        {
                std::optional<gui::dialog::ObjectSelectionParameters> selection =
                        gui::dialog::ObjectSelectionParametersDialog::show();
                if (!selection)
                {
                        return nullptr;
                }
                selection_parameters = std::move(*selection);
        }
        else
        {
                selection_parameters = gui::dialog::ObjectSelectionParametersDialog::current();
        }

        gui::dialog::BoundCoconeParameters bound_cocone_parameters =
                gui::dialog::BoundCoconeParametersDialog::current();

        return [=](ProgressRatioList* progress_list)
        {
                unsigned dimension = mesh::file_dimension(file_name);

                apply_for_dimension(
                        dimension,
                        [&]<size_t N>(const Dimension<N>&)
                        {
                                std::shared_ptr<mesh::MeshObject<N>> mesh = load_mesh_from_file<N>(
                                        generic_utf8_filename(file_name.filename()), progress_list, file_name);

                                compute<N>(
                                        progress_list, selection_parameters.convex_hull, selection_parameters.cocone,
                                        selection_parameters.bound_cocone, selection_parameters.mst, *mesh,
                                        bound_cocone_parameters.rho, bound_cocone_parameters.alpha);
                        });
        };
}

std::function<void(ProgressRatioList*)> action_load_mesh_from_repository(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name)
{
        if (object_name.empty())
        {
                MESSAGE_ERROR("Empty mesh repository object name");
                return nullptr;
        }

        std::optional<gui::dialog::PointObjectParameters> point_object_parameters =
                gui::dialog::PointObjectParametersDialog::show(
                        dimension, object_name, POINT_COUNT_DEFAULT, POINT_COUNT_MINIMUM, POINT_COUNT_MAXIMUM);
        if (!point_object_parameters)
        {
                return nullptr;
        }

        std::optional<gui::dialog::ObjectSelectionParameters> selection_parameters =
                gui::dialog::ObjectSelectionParametersDialog::show();
        if (!selection_parameters)
        {
                return nullptr;
        }

        gui::dialog::BoundCoconeParameters bound_cocone_parameters =
                gui::dialog::BoundCoconeParametersDialog::current();

        return [=](ProgressRatioList* progress_list)
        {
                apply_for_dimension(
                        dimension,
                        [&]<size_t N>(const Dimension<N>&)
                        {
                                std::shared_ptr<mesh::MeshObject<N>> mesh = load_mesh_from_repository<N>(
                                        object_name, point_object_parameters->point_count, *repository);

                                compute<N>(
                                        progress_list, selection_parameters->convex_hull, selection_parameters->cocone,
                                        selection_parameters->bound_cocone, selection_parameters->mst, *mesh,
                                        bound_cocone_parameters.rho, bound_cocone_parameters.alpha);
                        });
        };
}

std::function<void(ProgressRatioList*)> action_load_volume_from_file(std::filesystem::path path)
{
        if (path.empty())
        {
                std::string caption = "Open";
                bool read_only = true;

                std::optional<std::string> directory_string = gui::dialog::select_directory(caption, read_only);
                if (!directory_string)
                {
                        return nullptr;
                }
                path = path_from_utf8(*directory_string);
        }

        return [=](ProgressRatioList* progress_list)
        {
                std::vector<int> image_size = volume::find_size(path);

                apply_for_dimension(
                        image_size.size(),
                        [&]<size_t N>(const Dimension<N>&)
                        {
                                load_volume_from_file<N>(generic_utf8_filename(path.filename()), progress_list, path);
                        });
        };
}

std::function<void(ProgressRatioList*)> action_load_volume_from_repository(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name)
{
        if (object_name.empty())
        {
                MESSAGE_ERROR("Empty volume repository object name");
                return nullptr;
        }

        std::optional<gui::dialog::VolumeObjectParameters> parameters = gui::dialog::VolumeObjectParametersDialog::show(
                dimension, object_name, VOLUME_IMAGE_SIZE_DEFAULT, VOLUME_IMAGE_SIZE_MINIMUM,
                VOLUME_IMAGE_SIZE_MAXIMUM);
        if (!parameters)
        {
                return nullptr;
        }

        return [=](ProgressRatioList* /*progress_list*/)
        {
                apply_for_dimension(
                        dimension,
                        [&]<size_t N>(const Dimension<N>&)
                        {
                                load_volume_from_repository<N>(object_name, parameters->image_size, *repository);
                        });
        };
}
}
