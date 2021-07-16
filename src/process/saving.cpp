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

#include "saving.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/message.h>
#include <src/gui/dialogs/file_dialog.h>
#include <src/gui/dialogs/view_image.h>
#include <src/image/depth.h>
#include <src/image/file.h>
#include <src/model/mesh_utility.h>

namespace ns::process
{
namespace
{
constexpr bool STL_FORMAT_ASCII = true;

template <std::size_t N>
std::function<void(ProgressRatioList*)> action_save_function(
        const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object)
{
        std::string name = mesh_object->name();

        std::string caption = "Save " + name;
        bool read_only = true;

        std::vector<gui::dialog::FileFilter> filters;
        for (const mesh::FileFormat& v : mesh::save_formats(N))
        {
                gui::dialog::FileFilter& filter = filters.emplace_back();
                filter.name = v.format_name;
                filter.file_extensions = v.file_name_extensions;
        }

        std::optional<std::string> file_name_string = gui::dialog::save_file(caption, filters, read_only);
        if (!file_name_string)
        {
                return nullptr;
        }
        std::filesystem::path file_name = path_from_utf8(*file_name_string);

        mesh::FileType file_type = mesh::file_type_by_name(file_name);

        return [=](ProgressRatioList*)
        {
                mesh::Reading reading(*mesh_object);
                switch (file_type)
                {
                case mesh::FileType::Obj:
                        mesh::save_to_obj(reading.mesh(), file_name, name);
                        MESSAGE_INFORMATION(name + " saved to OBJ file " + generic_utf8_filename(file_name));
                        return;
                case mesh::FileType::Stl:
                        mesh::save_to_stl(reading.mesh(), file_name, name, STL_FORMAT_ASCII);
                        MESSAGE_INFORMATION(name + " saved to STL file " + generic_utf8_filename(file_name));
                        return;
                }
                error_fatal("Unknown file type for saving");
        };
}
}

std::function<void(ProgressRatioList*)> action_save(const storage::MeshObjectConst& object)
{
        return std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object)
                {
                        return action_save_function(mesh_object);
                },
                object);
}

std::function<void(ProgressRatioList*)> action_save(image::Image<2>&& image)
{
        const bool use_to_8_bit = 1 < (image::format_pixel_size_in_bytes(image.color_format)
                                       / image::format_component_count(image.color_format));

        std::optional<gui::dialog::ViewImageParameters> dialog_parameters =
                gui::dialog::ViewImageDialog::show("Save Image", use_to_8_bit);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        ASSERT(use_to_8_bit == dialog_parameters->convert_to_8_bit.has_value());

        return [parameters = std::move(*dialog_parameters),
                image = std::make_shared<image::Image<2>>(std::move(image))](ProgressRatioList*)
        {
                if (parameters.convert_to_8_bit && *parameters.convert_to_8_bit)
                {
                        *image = image::convert_to_8_bit(*image);
                }
                const std::filesystem::path file_name = path_from_utf8(parameters.path_string);
                image::save(file_name, image::ImageView<2>(*image));
                MESSAGE_INFORMATION("Image saved to " + generic_utf8_filename(file_name));
        };
}
}
