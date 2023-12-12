/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ns::model::mesh
{
int file_dimension(const std::filesystem::path& file_name)
{
        return std::get<0>(file::file_dimension_and_type(file_name));
}

//

std::string obj_file_extension(const std::size_t n)
{
        return (n == 3) ? "obj" : "obj" + to_string(n);
}

std::vector<std::string> obj_file_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> res;
        res.reserve(dimensions.size() + (dimensions.contains(3) ? 1 : 0));
        for (const unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                if (d == 3)
                {
                        res.emplace_back("obj");
                        res.emplace_back("obj3");
                }
                else
                {
                        res.push_back("obj" + to_string(d));
                }
        }
        return res;
}

bool file_has_obj_extension(const std::size_t n, const std::filesystem::path& file_name)
{
        const std::string extension = generic_utf8_filename(file_name.extension());
        return (extension == '.' + obj_file_extension(n)) || (extension == ".obj" + to_string(n));
}

//

std::string stl_file_extension(const std::size_t n)
{
        return (n == 3) ? "stl" : "stl" + to_string(n);
}

std::vector<std::string> stl_file_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> res;
        res.reserve(dimensions.size() + (dimensions.contains(3) ? 1 : 0));
        for (const unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                if (d == 3)
                {
                        res.emplace_back("stl");
                        res.emplace_back("stl3");
                }
                else
                {
                        res.push_back("stl" + to_string(d));
                }
        }
        return res;
}

bool file_has_stl_extension(const std::size_t n, const std::filesystem::path& file_name)
{
        const std::string extension = generic_utf8_filename(file_name.extension());
        return (extension == '.' + stl_file_extension(n)) || (extension == ".stl" + to_string(n));
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
                        return FileType::OBJ;
                }
                file::read_dimension_number(extension.substr(OBJ.size()));
                return FileType::OBJ;
        }

        if (extension.find(STL) == 0)
        {
                if (extension == STL)
                {
                        return FileType::STL;
                }
                file::read_dimension_number(extension.substr(STL.size()));
                return FileType::STL;
        }

        error("Failed to find the file type by its extension for the file name " + generic_utf8_filename(file_name));
}

//

std::vector<std::string> txt_file_extensions(const std::set<unsigned>& dimensions)
{
        std::vector<std::string> res;
        res.reserve(dimensions.size() + 1);
        res.emplace_back("txt");
        for (const unsigned d : dimensions)
        {
                ASSERT(d >= 3);
                res.push_back("txt" + to_string(d));
        }
        return res;
}
}
