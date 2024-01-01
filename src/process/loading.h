/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/progress/progress_list.h>
#include <src/storage/repository.h>

#include <filesystem>
#include <functional>
#include <string>

namespace ns::process
{
std::function<void(progress::RatioList*)> action_load_mesh(
        std::filesystem::path path,
        bool use_object_selection_dialog);

std::function<void(progress::RatioList*)> action_load_point_mesh(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name);

std::function<void(progress::RatioList*)> action_load_facet_mesh(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name);

std::function<void(progress::RatioList*)> action_load_volume(std::filesystem::path path);

std::function<void(progress::RatioList*)> action_load_volume(
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name);
}
