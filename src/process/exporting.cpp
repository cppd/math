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

#include "exporting.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/gui/dialogs/file_dialog.h>
#include <src/model/mesh_utility.h>
#include <src/utility/file/path.h>

namespace process
{
namespace
{
constexpr bool STL_EXPORT_FORMAT_ASCII = true;

template <size_t N>
std::function<void(ProgressRatioList*)> action_export_function(
        const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object)
{
        std::string name = mesh_object->name();

        std::string caption = "Export " + name + " to file";
        bool read_only = true;

        std::vector<gui::dialog::FileFilter> filters;
        for (const mesh::FileFormat& v : mesh::save_formats(N))
        {
                gui::dialog::FileFilter& filter = filters.emplace_back();
                filter.name = v.format_name;
                filter.file_extensions = v.file_name_extensions;
        }

        std::string file_name_string;
        if (!gui::dialog::save_file(caption, filters, read_only, &file_name_string))
        {
                return nullptr;
        }
        std::filesystem::path file_name = path_from_utf8(file_name_string);

        mesh::FileType file_type = mesh::file_type_by_name(file_name);

        return [=](ProgressRatioList*) {
                mesh::Reading reading(*mesh_object);
                switch (file_type)
                {
                case mesh::FileType::Obj:
                        mesh::save_to_obj(reading.mesh(), file_name, name);
                        MESSAGE_INFORMATION(name + " exported to OBJ file " + generic_utf8_filename(file_name));
                        return;
                case mesh::FileType::Stl:
                        mesh::save_to_stl(reading.mesh(), file_name, name, STL_EXPORT_FORMAT_ASCII);
                        MESSAGE_INFORMATION(name + " exported to STL file " + generic_utf8_filename(file_name));
                        return;
                }
                error_fatal("Unknown file type for export");
        };
}
}

std::function<void(ProgressRatioList*)> action_export(const storage::MeshObjectConst& object)
{
        return std::visit(
                [&]<size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object) {
                        return action_export_function(mesh_object);
                },
                object);
}
}
