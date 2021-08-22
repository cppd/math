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

#include "file_info.h"

#include "file/file_type.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>

namespace ns::mesh
{
int file_dimension(const std::filesystem::path& file_name)
{
        return std::get<0>(file::file_dimension_and_type(file_name));
}

//

std::string obj_file_extension(std::size_t n)
{
        return (n == 3) ? "obj" : "obj" + to_string(n);
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

bool file_has_obj_extension(std::size_t n, const std::filesystem::path& file_name)
{
        return (file_name.extension() == "." + obj_file_extension(n))
               || (file_name.extension() == ".obj" + to_string(n));
}

//

std::string stl_file_extension(std::size_t n)
{
        return (n == 3) ? "stl" : "stl" + to_string(n);
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

bool file_has_stl_extension(std::size_t n, const std::filesystem::path& file_name)
{
        return (file_name.extension() == "." + stl_file_extension(n))
               || (file_name.extension() == ".stl" + to_string(n));
}

//

FileType file_type_by_name(const std::filesystem::path& file_name)
{
        static constexpr std::string_view OBJ = ".obj";
        static constexpr std::string_view STL = ".stl";

        const std::string extension = generic_utf8_filename(file_name.extension());

        if (extension.find(OBJ) == 0)
        {
                if (extension == OBJ)
                {
                        return FileType::Obj;
                }
                file::read_dimension_number(extension.substr(OBJ.size()));
                return FileType::Obj;
        }

        if (extension.find(STL) == 0)
        {
                if (extension == STL)
                {
                        return FileType::Stl;
                }
                file::read_dimension_number(extension.substr(STL.size()));
                return FileType::Stl;
        }

        error("Failed to find the file type by its extension for the file name " + generic_utf8_filename(file_name));
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
