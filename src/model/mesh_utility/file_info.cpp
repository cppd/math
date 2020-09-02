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

#include "file_info.h"

#include "file/file_type.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/utility/file/sys.h>

namespace mesh
{
int file_dimension(const std::string& file_name)
{
        return std::get<0>(file::file_dimension_and_type(file_name));
}

//

std::string obj_file_extension(size_t N)
{
        return (N == 3) ? "obj" : "obj" + to_string(N);
}

std::vector<std::string> obj_file_extensions(const std::set<unsigned>& dimensions)
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

bool is_obj_file_extension(size_t N, const std::string& extension)
{
        return (extension == obj_file_extension(N)) || (extension == "obj" + to_string(N));
}

//

std::string stl_file_extension(size_t N)
{
        return (N == 3) ? "stl" : "stl" + to_string(N);
}

std::vector<std::string> stl_file_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> result;
        for (unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                if (d == 3)
                {
                        result.emplace_back("stl");
                        result.emplace_back("stl3");
                }
                else
                {
                        result.push_back("stl" + to_string(d));
                }
        }
        return result;
}

bool is_stl_file_extension(size_t N, const std::string& extension)
{
        return (extension == stl_file_extension(N)) || (extension == "stl" + to_string(N));
}

//

FileType file_type_by_extension(const std::string& file_name)
{
        const std::string ext = file_extension(file_name);
        const std::string OBJ = "obj";
        if (ext.find(OBJ) == 0)
        {
                if (ext == OBJ)
                {
                        return FileType::Obj;
                }
                file::read_dimension_number(ext.substr(OBJ.size()));
                return FileType::Obj;
        }
        const std::string STL = "stl";
        if (ext.find(STL) == 0)
        {
                if (ext == STL)
                {
                        return FileType::Stl;
                }
                file::read_dimension_number(ext.substr(STL.size()));
                return FileType::Stl;
        }
        error("Failed to find the file type by its extension for the file name " + file_name);
}

//

std::vector<std::string> txt_file_extensions(const std::set<unsigned>& dimensions)
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
}
