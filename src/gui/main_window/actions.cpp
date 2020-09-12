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

#include "actions.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/process/loading.h>

namespace gui
{
namespace
{
void load_mesh(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name)
{
        threads->terminate_and_start(WorkerThreads::Action::Work, "Load from mesh repository", [&]() {
                WorkerThreads::Function f =
                        process::action_load_from_mesh_repository(repository, dimension, object_name);
                //model_tree->clear();
                //view->send(view::command::ResetView());
                return f;
        });
}

void load_volume(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name)
{
        threads->terminate_and_start(WorkerThreads::Action::Work, "Load from volume repository", [&]() {
                return process::action_load_from_volume_repository(repository, dimension, object_name);
        });
}
}

Actions::Actions(QMenu* menu, const storage::Repository* repository, WorkerThreads* threads) : m_threads(threads)
{
        std::vector<storage::Repository::ObjectNames> repository_objects = repository->object_names();

        std::sort(repository_objects.begin(), repository_objects.end(), [](const auto& a, const auto& b) {
                return a.dimension < b.dimension;
        });

        for (storage::Repository::ObjectNames& objects : repository_objects)
        {
                const int dimension = objects.dimension;
                ASSERT(dimension > 0);

                QMenu* sub_menu = menu->addMenu(space_name(dimension).c_str());

                std::sort(objects.mesh_names.begin(), objects.mesh_names.end());
                for (const std::string& object_name : objects.mesh_names)
                {
                        ASSERT(!object_name.empty());

                        std::string action_text = object_name + "...";
                        QAction* action = sub_menu->addAction(action_text.c_str());
                        auto f = [threads, repository, dimension, object_name]() {
                                load_mesh(threads, repository, dimension, object_name);
                        };
                        m_connections.emplace_back(QObject::connect(action, &QAction::triggered, std::move(f)));
                }

                if (dimension == 3)
                {
                        sub_menu->addSeparator();

                        std::sort(objects.volume_names.begin(), objects.volume_names.end());
                        for (const std::string& object_name : objects.volume_names)
                        {
                                ASSERT(!object_name.empty());

                                std::string action_text = object_name + "...";
                                QAction* action = sub_menu->addAction(action_text.c_str());
                                auto f = [threads, repository, dimension, object_name]() {
                                        load_volume(threads, repository, dimension, object_name);
                                };
                                m_connections.emplace_back(QObject::connect(action, &QAction::triggered, std::move(f)));
                        }
                }
        }
}

void Actions::connect_load_action(QAction* action)
{
        m_connections.emplace_back(
                QObject::connect(action, &QAction::triggered, [this]() { load_from_file("", true); }));
}

void Actions::load_from_file(const std::string& file_name, bool use_object_selection_dialog)
{
        m_threads->terminate_and_start(WorkerThreads::Action::Work, "Loading from file", [&]() {
                WorkerThreads::Function f = process::action_load_from_file(file_name, use_object_selection_dialog);
                //model_tree->clear();
                //view->send(view::command::ResetView());
                return f;
        });
}
}
