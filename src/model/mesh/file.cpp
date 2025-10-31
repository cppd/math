/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "file.h"

#include "file_info.h"

#include "file/file_type.h"
#include "file/load_obj.h"
#include "file/load_stl.h"
#include "file/load_txt.h"
#include "file/save_obj.h"
#include "file/save_stl.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/model/mesh.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <filesystem>
#include <flat_set>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::model::mesh
{
std::vector<FileFormat> save_formats(const unsigned dimension)
{
        std::vector<FileFormat> v(2);

        v[0].format_name = "OBJ";
        v[0].file_name_extensions = {obj_file_extension(dimension)};

        v[1].format_name = "STL";
        v[1].file_name_extensions = {stl_file_extension(dimension)};

        return v;
}

std::vector<FileFormat> load_formats(const std::flat_set<unsigned>& dimensions)
{
        std::vector<FileFormat> v(1);

        v[0].format_name = "All Supported Formats";
        for (std::string& s : obj_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }
        for (std::string& s : stl_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }
        for (std::string& s : txt_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }

        return v;
}

//

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load(const Path& file_name, progress::Ratio* const progress)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        const auto [dimension, file_type] = file::file_dimension_and_type(file_name);

        if (dimension != static_cast<int>(N))
        {
                error("Requested file dimension " + to_string(N) + ", detected file dimension " + to_string(dimension)
                      + ", file " + generic_utf8_filename(file_name));
        }

        switch (file_type)
        {
        case file::MeshFileType::OBJ:
                return file::load_from_obj_file<N>(file_name, progress);
        case file::MeshFileType::STL:
                return file::load_from_stl_file<N>(file_name, progress);
        case file::MeshFileType::TXT:
                return file::load_from_txt_file<N>(file_name, progress);
        }

        error_fatal("Unknown file type");
}

template <std::size_t N, typename Path>
std::filesystem::path save_to_obj(const Mesh<N>& mesh, const Path& file_name, const std::string_view comment)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        if (!file_has_obj_extension(N, file_name))
        {
                error("Not OBJ file extension \"" + generic_utf8_filename(file_name) + "\" for saving to OBJ format, "
                      + space_name(N));
        }
        return file::save_to_obj_file(mesh, file_name, comment);
}

template <std::size_t N, typename Path>
std::filesystem::path save_to_stl(
        const Mesh<N>& mesh,
        const Path& file_name,
        const std::string_view comment,
        const bool ascii_format)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        if (!file_has_stl_extension(N, file_name))
        {
                error("Not STL file extension \"" + generic_utf8_filename(file_name) + "\" for saving to STL format, "
                      + space_name(N));
        }
        return file::save_to_stl_file(mesh, file_name, comment, ascii_format);
}

#define TEMPLATE(N)                                                                                                   \
        template std::unique_ptr<Mesh<(N)>> load(const std::filesystem::path&, progress::Ratio*);                     \
        template std::filesystem::path save_to_obj(const Mesh<(N)>&, const std::filesystem::path&, std::string_view); \
        template std::filesystem::path save_to_stl(                                                                   \
                const Mesh<(N)>&, const std::filesystem::path&, std::string_view, bool);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
