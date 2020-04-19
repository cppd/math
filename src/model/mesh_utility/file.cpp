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

#include "file.h"

#include "file_info.h"

#include "file/file_type.h"
#include "file/load_obj.h"
#include "file/load_stl.h"
#include "file/load_txt.h"
#include "file/save_obj.h"
#include "file/save_stl.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/utility/file/sys.h>

namespace mesh
{
std::vector<FileFormat> save_formats(unsigned dimension)
{
        std::vector<FileFormat> v(2);

        v[0].format_name = "OBJ";
        v[0].file_name_extensions = {mesh::obj_file_extension(dimension)};

        v[1].format_name = "STL";
        v[1].file_name_extensions = {mesh::stl_file_extension(dimension)};

        return v;
}

std::vector<FileFormat> load_formats(const std::set<unsigned>& dimensions)
{
        std::vector<FileFormat> v(1);

        v[0].format_name = "All Supported Formats";
        for (std::string& s : mesh::obj_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }
        for (std::string& s : mesh::stl_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }
        for (std::string& s : mesh::txt_file_extensions(dimensions))
        {
                v[0].file_name_extensions.push_back(std::move(s));
        }

        return v;
}

//

template <size_t N>
std::unique_ptr<Mesh<N>> load(const std::string& file_name, ProgressRatio* progress)
{
        auto [dimension, file_type] = file::file_dimension_and_type(file_name);

        if (dimension != static_cast<int>(N))
        {
                error("Requested file dimension " + to_string(N) + ", detected file dimension " + to_string(dimension)
                      + ", file " + file_name);
        }

        switch (file_type)
        {
        case file::MeshFileType::Obj:
                return file::load_from_obj_file<N>(file_name, progress);
        case file::MeshFileType::Stl:
                return file::load_from_stl_file<N>(file_name, progress);
        case file::MeshFileType::Txt:
                return file::load_from_txt_file<N>(file_name, progress);
        }

        error_fatal("Unknown file type");
}

template <size_t N>
std::string save_to_obj(const Mesh<N>& mesh, const std::string& file_name, const std::string_view& comment)
{
        std::string ext = file_extension(file_name);
        if (!is_obj_file_extension(N, ext))
        {
                error("Not OBJ file extension \"" + ext + "\" for saving to OBJ format, " + space_name(N));
        }
        return file::save_to_obj_file(mesh, file_name, comment);
}

template <size_t N>
std::string save_to_stl(
        const Mesh<N>& mesh,
        const std::string& file_name,
        const std::string_view& comment,
        bool ascii_format)
{
        std::string ext = file_extension(file_name);
        if (!is_stl_file_extension(N, ext))
        {
                error("Not STL file extension \"" + ext + "\" for saving to STL format, " + space_name(N));
        }
        return file::save_to_stl_file(mesh, file_name, comment, ascii_format);
}

//

template std::unique_ptr<Mesh<3>> load(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load(const std::string&, ProgressRatio*);

template std::string save_to_obj(const Mesh<3>&, const std::string&, const std::string_view&);
template std::string save_to_obj(const Mesh<4>&, const std::string&, const std::string_view&);
template std::string save_to_obj(const Mesh<5>&, const std::string&, const std::string_view&);
template std::string save_to_obj(const Mesh<6>&, const std::string&, const std::string_view&);

template std::string save_to_stl(const Mesh<3>&, const std::string&, const std::string_view&, bool);
template std::string save_to_stl(const Mesh<4>&, const std::string&, const std::string_view&, bool);
template std::string save_to_stl(const Mesh<5>&, const std::string&, const std::string_view&, bool);
template std::string save_to_stl(const Mesh<6>&, const std::string&, const std::string_view&, bool);
}
