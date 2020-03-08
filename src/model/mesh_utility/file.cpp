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

#include "file/file_type.h"
#include "file/load_obj.h"
#include "file/load_txt.h"
#include "file/save_obj.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/utility/file/sys.h>

namespace mesh
{
int file_dimension(const std::string& file_name)
{
        return std::get<0>(file::file_dimension_and_type(file_name));
}

std::string obj_file_extension(size_t N)
{
        return (N == 3) ? "obj" : "obj" + to_string(N);
}

std::vector<std::string> obj_file_supported_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> result;
        for (unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                if (d == 3)
                {
                        result.emplace_back("obj");
                        result.emplace_back("obj3");
                }
                else
                {
                        result.push_back("obj" + to_string(d));
                }
        }
        return result;
}

std::vector<std::string> txt_file_supported_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> result;
        result.emplace_back("txt");
        for (unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                result.push_back("txt" + to_string(d));
        }
        return result;
}

bool is_obj_file_extension(size_t N, const std::string& extension)
{
        return (extension == obj_file_extension(N)) || (extension == "obj" + to_string(N));
}

//

template <size_t N>
std::unique_ptr<Mesh<N>> load_geometry(const std::string& file_name, ProgressRatio* progress)
{
        auto [dimension, file_type] = file::file_dimension_and_type(file_name);

        if (dimension != static_cast<int>(N))
        {
                error("Requested file dimension " + to_string(N) + ", detected file dimension " + to_string(dimension) +
                      ", file " + file_name);
        }

        switch (file_type)
        {
        case file::ObjFileType::Obj:
                return file::load_from_obj_file<N>(file_name, progress);
        case file::ObjFileType::Txt:
                return file::load_from_txt_file<N>(file_name, progress);
        }

        error_fatal("Unknown file type");
}

template <size_t N>
std::string save_geometry(const Mesh<N>& mesh, const std::string& file_name, const std::string_view& comment)
{
        std::string ext = file_extension(file_name);
        if (is_obj_file_extension(N, ext))
        {
                return file::save_to_obj_file(mesh, file_name, comment);
        }
        if (!ext.empty())
        {
                error("Unsupported format " + file_name);
        }
        error("Empty extension " + file_name);
}

template std::string save_geometry(const Mesh<3>&, const std::string&, const std::string_view&);
template std::string save_geometry(const Mesh<4>&, const std::string&, const std::string_view&);
template std::string save_geometry(const Mesh<5>&, const std::string&, const std::string_view&);
template std::string save_geometry(const Mesh<6>&, const std::string&, const std::string_view&);

template std::unique_ptr<Mesh<3>> load_geometry(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load_geometry(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load_geometry(const std::string&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load_geometry(const std::string&, ProgressRatio*);
}
