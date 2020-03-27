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

#pragma once

#include <set>
#include <string>
#include <vector>

namespace mesh
{
int file_dimension(const std::string& file_name);

std::string obj_file_extension(size_t N);
std::vector<std::string> obj_file_extensions(const std::set<unsigned>& dimensions);
bool is_obj_file_extension(size_t N, const std::string& e);

std::string stl_file_extension(size_t N);
std::vector<std::string> stl_file_extensions(const std::set<unsigned>& dimensions);
bool is_stl_file_extension(size_t N, const std::string& e);

std::vector<std::string> txt_file_extensions(const std::set<unsigned>& dimensions);
}
