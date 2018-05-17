/*
Copyright (C) 2017, 2018 Topological Manifold

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

// Для 3 измерений расширения obj, obj3, txt, txt3.
// Для 4 и более измерений objN, txt, txtN.
// Если число указано, то используется оно. Если только txt,
// то подсчитывается количество чисел в первой строке файла.

#include "obj_file.h"

#include "com/error.h"
#include "com/file/file_sys.h"
#include "com/print.h"
#include "com/string/ascii.h"
#include "com/string/str.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace
{
// Чтение первой строки файла с ограничением на максимальное количество символов
std::string read_first_line_from_file(const std::string& file_name, int max_char_count)
{
        std::ifstream f(file_name, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + file_name);
        }

        int char_count = 0;
        char c;
        std::string line;

        while (f.get(c) && c != '\n' && ++char_count <= max_char_count)
        {
                line += c;
        }

        if (!f)
        {
                error("Failed to read line with endline character from file " + file_name);
        }

        if (char_count > max_char_count)
        {
                error("The first file line is too long (limit " + to_string(max_char_count) + "), file " + file_name);
        }

        return line;
}

// Чтение чисел из строки с проверкой, что имеются только числа и пробелы
int count_numbers(const std::string& s)
{
        std::istringstream iss(s);

        int d = 0;

        while (iss)
        {
                long double tmp;
                iss >> tmp;
                if (iss)
                {
                        ++d;
                }
        }

        if (d == 0)
        {
                error("Failed to read a floating point number from string \"" + s + "\"");
        }

        iss.clear();

        std::string tmp;
        iss >> tmp;
        if (iss)
        {
                error("Failed to find dimension number from string \"" + s + "\"");
        }

        return d;
}

// Определение размерности по количеству чисел в первой строке файла
int count_numbers_in_file(const std::string& file_name)
{
        std::string line = read_first_line_from_file(file_name, 1000000);

        if (line.empty())
        {
                error("The first line of the file is empty, file " + file_name);
        }

        return count_numbers(line);
}

// Чтение числа из строки с проверкой, что строка содержит только целое число без других символов
int read_dimension_number(const std::string& s)
{
        if (!std::all_of(s.cbegin(), s.cend(), [](char c) { return ascii::is_digit(c); }))
        {
                error("Wrong dimension number string \"" + s + "\"");
        }

        std::istringstream iss(s);

        int d;

        iss >> d;
        if (!iss)
        {
                error("Failed to read dimension number from string \"" + s + "\"");
        }

        char tmp;
        if (iss.get(tmp))
        {
                error("Wrong dimension number string \"" + s + "\"");
        }

        return d;
}
}

std::tuple<int, ObjFileType> obj_file_dimension_and_type(const std::string& file_name)
{
        // Если не только obj или txt, то после obj или txt должно быть целое
        // число и только целое число, например, строка "obj4" или "txt4"

        const std::string OBJ = "obj";
        const std::string TXT = "txt";

        std::string e = file_extension(file_name);

        if (e.empty())
        {
                error("No OBJ file extension found");
        }

        int dimension;
        ObjFileType obj_file_type;

        if (e == TXT)
        {
                dimension = count_numbers_in_file(file_name);
                obj_file_type = ObjFileType::Txt;
        }
        else if (e == OBJ)
        {
                dimension = 3;
                obj_file_type = ObjFileType::Obj;
        }
        else if (e.find(OBJ) == 0)
        {
                dimension = read_dimension_number(e.substr(OBJ.size()));
                obj_file_type = ObjFileType::Obj;
        }
        else if (e.find(TXT) == 0)
        {
                dimension = read_dimension_number(e.substr(TXT.size()));
                int dimension_numbers = count_numbers_in_file(file_name);
                if (dimension != dimension_numbers)
                {
                        error("Conflicting dimensions in file extension " + to_string(dimension) + " and in file data " +
                              to_string(dimension_numbers));
                }
                obj_file_type = ObjFileType::Txt;
        }
        else
        {
                error("Unsupported OBJ file format " + e);
        }

        if (dimension < 3)
        {
                error("Wrong dimension number " + to_string(dimension));
        }

        return {dimension, obj_file_type};
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
                };
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
