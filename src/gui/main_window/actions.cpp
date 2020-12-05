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
#include <src/com/message.h>
#include <src/com/names.h>
#include <src/process/computing.h>
#include <src/process/exporting.h>
#include <src/process/loading.h>
#include <src/process/painting.h>
#include <src/process/testing.h>

namespace gui::main_window
{
namespace
{
constexpr unsigned WORKER_THREAD_ID = 0;
constexpr unsigned SELF_TEST_THREAD_ID = 1;
constexpr unsigned REQUIRED_THREAD_COUNT = 2;

void load_mesh(
        WorkerThreads* threads,
        const storage::Repository* repository,
        int dimension,
        const std::string& object_name)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Load mesh from repository",
                [&]()
                {
                        WorkerThreads::Function f = process::action_load_mesh(repository, dimension, object_name);
                        //model_tree->clear();
                        //view->send(view::command::ResetView());
                        return f;
                });
}

void load_mesh(WorkerThreads* threads, const std::filesystem::path& path, bool use_object_selection_dialog)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Load mesh from filesystem",
                [&]()
                {
                        WorkerThreads::Function f = process::action_load_mesh(path, use_object_selection_dialog);
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
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Load volume from repository",
                [&]()
                {
                        return process::action_load_volume(repository, dimension, object_name);
                });
}

void load_volume(WorkerThreads* threads, const std::filesystem::path& path)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Load volume from filesystem",
                [&]()
                {
                        return process::action_load_volume(path);
                });
}

void export_mesh(WorkerThreads* threads, const ModelTree* model_tree)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Export",
                [&]()
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                MESSAGE_WARNING("No mesh to export");
                                return WorkerThreads::Function();
                        }
                        return process::action_export(*object);
                });
}

void painter(WorkerThreads* threads, const ModelTree* model_tree, view::View* view, const ColorsWidget* colors)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "Painter",
                [&]()
                {
                        std::vector<storage::MeshObjectConst> objects = model_tree->const_mesh_objects();
                        if (objects.empty())
                        {
                                MESSAGE_WARNING("No objects to paint");
                                return WorkerThreads::Function();
                        }
                        view::info::Camera camera;
                        view->receive({&camera});
                        return process::action_painter(
                                objects, camera, colors->background_color(), colors->lighting_intensity());
                });
}

void bound_cocone(WorkerThreads* threads, const ModelTree* model_tree)
{
        threads->terminate_and_start(
                WORKER_THREAD_ID, "BoundCocone",
                [&]()
                {
                        std::optional<storage::MeshObjectConst> object = model_tree->current_mesh_const();
                        if (!object)
                        {
                                MESSAGE_WARNING("No object to compute BoundCocone");
                                return WorkerThreads::Function();
                        }
                        return process::action_bound_cocone(*object);
                });
}

void self_test(WorkerThreads* threads, test::SelfTestType test_type, bool with_confirmation)
{
        threads->terminate_and_start(
                SELF_TEST_THREAD_ID, "Self-Test",
                [&]()
                {
                        return process::action_self_test(test_type, with_confirmation);
                });
}
}

Actions::Actions(
        const CommandLineOptions& options,
        QStatusBar* status_bar,
        QAction* action_self_test,
        QMenu* menu_file,
        QMenu* menu_create,
        QMenu* menu_edit,
        QMenu* menu_rendering,
        const storage::Repository* repository,
        view::View* view,
        ModelTree* model_tree,
        const ColorsWidget* colors)
{
        m_worker_threads = create_worker_threads(REQUIRED_THREAD_COUNT, SELF_TEST_THREAD_ID, status_bar);

        m_connections.emplace_back(QObject::connect(
                menu_file->addAction("Load Mesh..."), &QAction::triggered,
                [threads = m_worker_threads.get()]()
                {
                        load_mesh(threads, "", true);
                }));

        m_connections.emplace_back(QObject::connect(
                menu_file->addAction("Load Volume..."), &QAction::triggered,
                [threads = m_worker_threads.get()]()
                {
                        load_volume(threads, "");
                }));

        m_connections.emplace_back(QObject::connect(
                menu_file->addAction("Export..."), &QAction::triggered,
                [threads = m_worker_threads.get(), model_tree]()
                {
                        export_mesh(threads, model_tree);
                }));

        m_connections.emplace_back(QObject::connect(
                action_self_test, &QAction::triggered,
                [threads = m_worker_threads.get()]()
                {
                        self_test(threads, test::SelfTestType::Extended, true);
                }));

        m_connections.emplace_back(QObject::connect(
                menu_rendering->addAction("Painter..."), &QAction::triggered,
                [threads = m_worker_threads.get(), model_tree, view, colors]()
                {
                        painter(threads, model_tree, view, colors);
                }));

        m_connections.emplace_back(QObject::connect(
                menu_edit->addAction("BoundCocone..."), &QAction::triggered,
                [threads = m_worker_threads.get(), model_tree]()
                {
                        bound_cocone(threads, model_tree);
                }));

        std::vector<storage::Repository::ObjectNames> repository_objects = repository->object_names();
        std::sort(
                repository_objects.begin(), repository_objects.end(),
                [](const auto& a, const auto& b)
                {
                        return a.dimension < b.dimension;
                });
        for (storage::Repository::ObjectNames& objects : repository_objects)
        {
                const int dimension = objects.dimension;
                ASSERT(dimension > 0);

                QMenu* sub_menu = menu_create->addMenu(space_name(dimension).c_str());

                std::sort(objects.mesh_names.begin(), objects.mesh_names.end());
                for (const std::string& object_name : objects.mesh_names)
                {
                        ASSERT(!object_name.empty());

                        std::string action_text = object_name + "...";
                        QAction* action = sub_menu->addAction(action_text.c_str());
                        auto f = [threads = m_worker_threads.get(), repository, dimension, object_name]()
                        {
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
                                auto f = [threads = m_worker_threads.get(), repository, dimension, object_name]()
                                {
                                        load_volume(threads, repository, dimension, object_name);
                                };
                                m_connections.emplace_back(QObject::connect(action, &QAction::triggered, std::move(f)));
                        }
                }
        }

        self_test(m_worker_threads.get(), test::SelfTestType::Essential, false);

        if (!options.file_name.empty())
        {
                load_mesh(m_worker_threads.get(), options.file_name, !options.no_object_selection_dialog);
        }
}

Actions::~Actions()
{
        m_worker_threads->terminate_all();
}

void Actions::set_progresses()
{
        m_worker_threads->set_progresses();
}
}
