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

#include "file_type.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>

// 3 dimension: obj, obj3, stl, stl3, txt, txt3.
// 4 and more dimensions: objN, stlN, txt, txtN.
// if number is specified then use it.
// to find txt dimensions count numbers in the first line.

namespace ns::model::mesh::file
{
namespace
{
std::string read_first_line_from_file(const std::filesystem::path& file_name, int max_char_count)
{
        std::ifstream f(file_name, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(file_name));
        }

        int char_count = 0;
        char c = 0;
        std::string line;

        while (f.get(c) && c != '\n' && ++char_count <= max_char_count)
        {
                line += c;
        }

        if (!f)
        {
                error("Failed to read line with endline character from file " + generic_utf8_filename(file_name));
        }

        if (char_count > max_char_count)
        {
                error("The first file line is too long (limit " + to_string(max_char_count) + "), file "
                      + generic_utf8_filename(file_name));
        }

        return line;
}

int count_numbers(const std::string& s)
{
        std::istringstream iss(s);

        int count = 0;

        while (iss)
        {
                if (long double tmp; iss >> tmp)
                {
                        ++count;
                }
        }

        if (count == 0)
        {
                error("Failed to read a floating point number from string \"" + s + "\"");
        }

        iss.clear();

        if (std::string tmp; iss >> tmp)
        {
                error("Failed to find dimension number from string \"" + s + "\"");
        }

        return count;
}

int count_numbers_in_file(const std::filesystem::path& file_name)
{
        const std::string line = read_first_line_from_file(file_name, 1000000);

        if (line.empty())
        {
                error("The first line of the file is empty, file " + generic_utf8_filename(file_name));
        }

        return count_numbers(line);
}

template <typename Path>
std::tuple<int, MeshFileType> dimension_and_file_type(const Path& file_name)
{
        static constexpr std::string_view OBJ = ".obj";
        static constexpr std::string_view STL = ".stl";
        static constexpr std::string_view TXT = ".txt";

        const std::string extension = generic_utf8_filename(file_name.extension());

        if (extension.empty() || extension[0] != '.')
        {
                error("No file extension found");
        }

        if (extension.find(OBJ) == 0)
        {
                return {(extension == OBJ) ? 3 : read_dimension_number(extension.substr(OBJ.size())),
                        MeshFileType::OBJ};
        }

        if (extension.find(STL) == 0)
        {
                return {(extension == STL) ? 3 : read_dimension_number(extension.substr(STL.size())),
                        MeshFileType::STL};
        }

        if (extension.find(TXT) == 0)
        {
                if (extension == TXT)
                {
                        return {count_numbers_in_file(file_name), MeshFileType::TXT};
                }

                const int dimension = read_dimension_number(extension.substr(TXT.size()));
                const int dimension_numbers = count_numbers_in_file(file_name);
                if (dimension != dimension_numbers)
                {
                        error("Conflicting dimensions in file extension " + to_string(dimension) + " and in file data "
                              + to_string(dimension_numbers));
                }

                return {dimension, MeshFileType::TXT};
        }

        error("Unsupported file format " + extension);
}
}

int read_dimension_number(const std::string& s)
{
        const char* const first = s.data();
        const char* const last = s.data() + s.size();

        int number;
        const auto [ptr, ec] = std::from_chars(first, last, number);

        if (ec == std::errc{} && ptr == last && number > 0)
        {
                return number;
        }

        error("Failed to read dimension number from string \"" + s + "\"");
}

template <typename Path>
std::tuple<int, MeshFileType> file_dimension_and_type(const Path& file_name)
{
        const auto [dimension, file_type] = dimension_and_file_type(file_name);

        if (dimension < 3)
        {
                error("Wrong dimension number " + to_string(dimension));
        }

        return {dimension, file_type};
}

template std::tuple<int, MeshFileType> file_dimension_and_type(const std::filesystem::path&);
}
