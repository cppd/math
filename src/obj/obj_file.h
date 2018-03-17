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

#pragma once

#include <string>
#include <tuple>

enum class ObjFileType
{
        OBJ,
        TXT
};

std::tuple<int, ObjFileType> obj_file_dimension_and_type(const std::string& file_name);

std::string obj_file_name_extension(size_t N);
bool obj_file_name_extension_is_correct(size_t N, const std::string& e);
