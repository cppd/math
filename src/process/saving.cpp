/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "saving.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/message.h>
#include <src/com/print.h>
#include <src/gui/dialogs/file_dialog.h>
#include <src/gui/dialogs/view_image.h>
#include <src/image/depth.h>
#include <src/image/file_save.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/image/max.h>
#include <src/image/normalize.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/progress/progress_list.h>
#include <src/storage/types.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ns::process
{
namespace
{
constexpr bool STL_FORMAT_ASCII = true;

template <std::size_t N>
std::vector<gui::dialogs::FileFilter> create_filters()
{
        std::vector<gui::dialogs::FileFilter> res;
        for (const model::mesh::FileFormat& v : model::mesh::save_formats(N))
        {
                gui::dialogs::FileFilter& filter = res.emplace_back();
                filter.name = v.format_name;
                filter.file_extensions = v.file_name_extensions;
        }
        return res;
}

template <std::size_t N>
std::function<void(progress::RatioList*)> action_save_function(
        const std::shared_ptr<const model::mesh::MeshObject<N>>& mesh_object)
{
        const std::string name = mesh_object->name();

        const std::string caption = "Save " + name;
        const bool read_only = true;

        const std::vector<gui::dialogs::FileFilter> filters = create_filters<N>();

        const std::optional<std::string> file_name_string = gui::dialogs::save_file(caption, filters, read_only);
        if (!file_name_string)
        {
                return nullptr;
        }
        const std::filesystem::path file_name = path_from_utf8(*file_name_string);

        const model::mesh::FileType file_type = model::mesh::file_type_by_name(file_name);

        return [=](progress::RatioList*)
        {
                const model::mesh::Reading reading(*mesh_object);
                switch (file_type)
                {
                case model::mesh::FileType::OBJ:
                        model::mesh::save_to_obj(reading.mesh(), file_name, name);
                        message_information(name + " saved to OBJ file " + generic_utf8_filename(file_name));
                        return;
                case model::mesh::FileType::STL:
                        model::mesh::save_to_stl(reading.mesh(), file_name, name, STL_FORMAT_ASCII);
                        message_information(name + " saved to STL file " + generic_utf8_filename(file_name));
                        return;
                }
                error_fatal("Unknown file type for saving");
        };
}

std::string time_to_file_name(const std::chrono::system_clock::time_point& time)
{
        const std::tm t = time_to_local_time(time);
        std::ostringstream oss;
        oss << "image_" << std::put_time(&t, "%Y-%m-%d_%H-%M-%S");
        return oss.str();
}

std::string image_info(const image::Image<2>& image)
{
        const auto max = image::max(image.color_format, image.pixels);
        if (!max)
        {
                error("Maximum image value is not found");
        }
        if (!(std::isfinite(*max) && *max >= 0))
        {
                error("Error maximum image value " + to_string(*max));
        }
        return "Maximum: " + to_string(static_cast<float>(*max));
}
}

std::function<void(progress::RatioList*)> action_save(const storage::MeshObjectConst& object)
{
        return std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const model::mesh::MeshObject<N>>& mesh_object)
                {
                        return action_save_function(mesh_object);
                },
                object);
}

std::function<void(progress::RatioList*)> action_save(
        const std::chrono::system_clock::time_point& image_time,
        image::Image<2>&& image)
{
        if (image.color_format != image::ColorFormat::R32G32B32)
        {
                error("Unsupported color format " + image::format_to_string(image.color_format));
        }

        std::optional<gui::dialogs::ViewImageParameters> dialog_parameters =
                gui::dialogs::ViewImageDialog::show("Save Image", image_info(image), time_to_file_name(image_time));
        if (!dialog_parameters)
        {
                return nullptr;
        }

        return [parameters = std::move(*dialog_parameters),
                image = std::make_shared<image::Image<2>>(std::move(image))](progress::RatioList*)
        {
                if (parameters.normalize)
                {
                        image::normalize(image->color_format, &image->pixels);
                }

                if (parameters.convert_to_8_bit)
                {
                        *image = image::convert_to_8_bit(*image);
                }

                image::save(path_from_utf8(parameters.path_string), image::ImageView<2>(*image));
        };
}
}
