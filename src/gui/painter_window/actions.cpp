/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/message.h>

namespace ns::gui::painter_window
{
namespace
{
enum ThreadId
{
        SAVE_THREAD_ID,
        ADD_THREAD_ID,
        THREAD_ID_COUNT
};

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

Actions::Actions(const Pixels* pixels, QMenu* menu, QStatusBar* status_bar, std::function<long long()> slice_number)
        : m_pixels(pixels), m_worker_threads(create_worker_threads(THREAD_ID_COUNT, std::nullopt, status_bar))
{
        ASSERT(slice_number);
        {
                QAction* action = menu->addAction("Save...");
                m_connections.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [this, action, slice_number = std::move(slice_number)]()
                        {
                                save_image(action_name(action), slice_number());
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
                                        save_image(action_name(action));
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

void Actions::save_image(const std::string& action, long long slice) const
{
        std::optional<Pixels::Images> images = m_pixels->slice(slice);
        if (!images)
        {
                MESSAGE_WARNING("Image is not yet available");
                return;
        }
        if (images->size.size() != 2)
        {
                MESSAGE_WARNING("Error 2D image dimension " + to_string(images->size.size()));
                return;
        }

        m_worker_threads->terminate_and_start(
                SAVE_THREAD_ID, action,
                [&]()
                {
                        return painter_window::save_image(
                                images->size[0], images->size[1], images->rgb.color_format,
                                std::move(images->rgb.pixels), images->rgba.color_format,
                                std::move(images->rgba.pixels));
                });
}

void Actions::save_image(const std::string& action) const
{
        std::optional<Pixels::Images> images = m_pixels->pixels();
        if (!images)
        {
                MESSAGE_WARNING("Image is not yet available");
                return;
        }

        m_worker_threads->terminate_and_start(
                SAVE_THREAD_ID, action,
                [&]()
                {
                        return painter_window::save_image(
                                images->size, images->rgb.color_format, std::move(images->rgb.pixels),
                                images->rgba.color_format, std::move(images->rgba.pixels));
                });
}

void Actions::add_volume(const std::string& action) const
{
        std::optional<Pixels::Images> images = m_pixels->pixels();
        if (!images)
        {
                MESSAGE_WARNING("Image is not yet available");
                return;
        }

        m_worker_threads->terminate_and_start(
                ADD_THREAD_ID, action,
                [&]()
                {
                        return painter_window::add_volume(
                                images->size, images->rgb.color_format, std::move(images->rgb.pixels),
                                images->rgba.color_format, std::move(images->rgba.pixels));
                });
}
}
