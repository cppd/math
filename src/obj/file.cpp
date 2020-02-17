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

int obj_file_dimension(const std::string& file_name)
{
        return std::get<0>(obj_file_dimension_and_type(file_name));
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

bool obj_file_extension_is_correct(size_t N, const std::string& extension)
{
        return (extension == obj_file_extension(N)) || (extension == "obj" + to_string(N));
}

//

template <size_t N>
std::string save_obj_geometry_to_file(const Obj<N>* obj, const std::string& file_name, const std::string_view& comment)
{
        return save_obj(obj, file_name, comment);
}

template <size_t N>
std::unique_ptr<Obj<N>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress)
{
        auto [obj_dimension, obj_file_type] = obj_file_dimension_and_type(file_name);

        if (obj_dimension != static_cast<int>(N))
        {
                error("Requested OBJ file dimension " + to_string(N) + ", detected OBJ file dimension " +
                      to_string(obj_dimension) + ", file " + file_name);
        }

        switch (obj_file_type)
        {
        case ObjFileType::Obj:
                return load_obj<N>(file_name, progress);
        case ObjFileType::Txt:
                return load_txt<N>(file_name, progress);
        }

        error_fatal("Unknown obj file type");
}

template std::unique_ptr<Obj<3>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<4>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<5>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<6>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);

template std::string save_obj_geometry_to_file(
        const Obj<3>* obj,
        const std::string& file_name,
        const std::string_view& comment);

template std::string save_obj_geometry_to_file(
        const Obj<4>* obj,
        const std::string& file_name,
        const std::string_view& comment);

template std::string save_obj_geometry_to_file(
        const Obj<5>* obj,
        const std::string& file_name,
        const std::string_view& comment);

template std::string save_obj_geometry_to_file(
        const Obj<6>* obj,
        const std::string& file_name,
        const std::string_view& comment);
