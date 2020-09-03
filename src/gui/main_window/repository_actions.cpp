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

#include "repository_actions.h"

#include <src/com/error.h>
#include <src/com/names.h>

namespace gui
{
RepositoryActions::RepositoryActions(QMenu* menu, const storage::Repository& repository)
{
        std::vector<storage::Repository::ObjectNames> repository_objects = repository.object_names();

        std::sort(repository_objects.begin(), repository_objects.end(), [](const auto& a, const auto& b) {
                return a.dimension < b.dimension;
        });

        for (storage::Repository::ObjectNames& objects : repository_objects)
        {
                ASSERT(objects.dimension > 0);

                QMenu* sub_menu = menu->addMenu(space_name(objects.dimension).c_str());

                std::sort(objects.mesh_names.begin(), objects.mesh_names.end());
                for (const std::string& object_name : objects.mesh_names)
                {
                        ASSERT(!object_name.empty());

                        std::string action_text = object_name + "...";
                        QAction* action = sub_menu->addAction(action_text.c_str());

                        RepositoryActionDescription action_description;
                        action_description.dimension = objects.dimension;
                        action_description.object_name = object_name;

                        m_repository_actions.try_emplace(action, action_description);
                        connect(action, &QAction::triggered, this, &RepositoryActions::on_mesh_triggered);
                }

                if (objects.dimension == 3)
                {
                        sub_menu->addSeparator();

                        std::sort(objects.volume_names.begin(), objects.volume_names.end());
                        for (const std::string& object_name : objects.volume_names)
                        {
                                ASSERT(!object_name.empty());

                                std::string action_text = object_name + "...";
                                QAction* action = sub_menu->addAction(action_text.c_str());

                                RepositoryActionDescription action_description;
                                action_description.dimension = objects.dimension;
                                action_description.object_name = object_name;

                                m_repository_actions.try_emplace(action, action_description);
                                connect(action, &QAction::triggered, this, &RepositoryActions::on_volume_triggered);
                        }
                }
        }
}

void RepositoryActions::on_mesh_triggered()
{
        auto iter = m_repository_actions.find(sender());
        if (iter == m_repository_actions.cend())
        {
                return;
        }

        int dimension = iter->second.dimension;
        std::string object_name = iter->second.object_name;

        emit mesh(dimension, object_name);
}

void RepositoryActions::on_volume_triggered()
{
        auto iter = m_repository_actions.find(sender());
        if (iter == m_repository_actions.cend())
        {
                return;
        }

        int dimension = iter->second.dimension;
        std::string object_name = iter->second.object_name;

        emit volume(dimension, object_name);
}
}
