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

#include "process.h"

namespace gui::painter_window
{
namespace
{
constexpr unsigned SAVE_THREAD_ID = 0;
constexpr unsigned ADD_THREAD_ID = 1;
constexpr unsigned REQUIRED_THREAD_COUNT = 2;
constexpr unsigned PERMANENT_THREAD_ID = limits<unsigned>::max();

std::string action_name(const QAction* action)
{
        std::string s = action->text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}
}

Actions::Actions(const Pixels* pixels, QMenu* menu, QStatusBar* status_bar)
        : m_pixels(pixels),
          m_worker_threads(create_worker_threads(REQUIRED_THREAD_COUNT, PERMANENT_THREAD_ID, status_bar))
{
        {
                QAction* action = menu->addAction("Save...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [=, this]()
                        {
                                save_to_file(action_name(action));
                        }));
        }

        if (m_pixels->screen_size().size() >= 3)
        {
                {
                        QAction* action = menu->addAction("Save all...");
                        m_connections.emplace_back(QObject::connect(
                                action, &QAction::triggered,
                                [=, this]()
                                {
                                        save_all_to_files(action_name(action));
                                }));
                }

                menu->addSeparator();

                {
                        QAction* action = menu->addAction("Add volume...");
                        m_connections.emplace_back(QObject::connect(
                                action, &QAction::triggered,
                                [=, this]()
                                {
                                        add_volume(action_name(action));
                                }));
                }
        }
}

Actions::~Actions()
{
        m_connections.clear();
        m_worker_threads->terminate_all();
}

void Actions::set_progresses()
{
        m_worker_threads->set_progresses();
}

void Actions::save_to_file(const std::string& action) const
{
        std::vector<std::byte> slice = m_pixels->slice();

        m_worker_threads->terminate_and_start(
                SAVE_THREAD_ID, action,
                [&]()
                {
                        return painter_window::save_to_file(
                                m_pixels->screen_size(), m_pixels->background_color(), m_pixels->color_format(),
                                std::move(slice));
                });
}

void Actions::save_all_to_files(const std::string& action) const
{
        std::vector<std::byte> pixels = m_pixels->pixels();

        m_worker_threads->terminate_and_start(
                SAVE_THREAD_ID, action,
                [&]()
                {
                        return painter_window::save_all_to_files(
                                m_pixels->screen_size(), m_pixels->background_color(), m_pixels->color_format(),
                                std::move(pixels));
                });
}

void Actions::add_volume(const std::string& action) const
{
        std::vector<std::byte> pixels = m_pixels->pixels();

        m_worker_threads->terminate_and_start(
                ADD_THREAD_ID, action,
                [&]()
                {
                        return painter_window::add_volume(
                                m_pixels->screen_size(), m_pixels->background_color(), m_pixels->color_format(),
                                std::move(pixels));
                });
}
}
