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

#include <src/storage/repository.h>

#include <QMenu>
#include <QObject>
#include <string>
#include <unordered_map>

namespace gui
{
class RepositoryActions final : public QObject
{
        Q_OBJECT
public:
        RepositoryActions(QMenu* menu, const storage::Repository& repository);

Q_SIGNALS:
        void mesh(int dimension, const std::string& object_name);
        void volume(int dimension, const std::string& object_name);

private:
        void on_mesh_triggered();
        void on_volume_triggered();

        struct RepositoryActionDescription final
        {
                int dimension;
                std::string object_name;
        };
        std::unordered_map<QObject*, RepositoryActionDescription> m_repository_actions;
};
}
