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

#include "../com/connection.h"

#include <src/storage/repository.h>

#include <QMenu>
#include <functional>
#include <string>

namespace gui
{
class RepositoryActions final
{
        std::vector<Connection> m_connections;

        std::function<void(int dimension, const std::string& object_name)> m_on_mesh;
        std::function<void(int dimension, const std::string& object_name)> m_on_volume;

public:
        RepositoryActions(
                QMenu* menu,
                const storage::Repository& repository,
                std::function<void(int dimension, const std::string& object_name)>&& on_mesh,
                std::function<void(int dimension, const std::string& object_name)>&& on_volume);
};
}
