/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "pixels.h"
#include "process.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/com/print.h>
#include <src/gui/com/threads.h>

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QStatusBar>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

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

std::string action_name(const QAction& action)
{
        std::string s = action.text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}
}

Actions::Actions(
        const Pixels* const pixels,
        QMenu* const menu,
        QStatusBar* const status_bar,
        std::function<long long()> slice_number)
        : pixels_(pixels),
          worker_threads_(com::create_worker_threads(THREAD_ID_COUNT, std::nullopt, status_bar))
{
        ASSERT(slice_number);

        QAction* action = nullptr;

        action = menu->addAction("Save...");
        connections_.emplace_back(QObject::connect(
                action, &QAction::triggered,
                [this, action, slice_number = std::move(slice_number)]()
                {
                        save_image(action_name(*action), slice_number());
                }));

        if (pixels_->screen_size().size() >= 3)
        {
                action = menu->addAction("Save all...");
                connections_.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [action, this]()
                        {
                                save_image(action_name(*action));
                        }));

                menu->addSeparator();

                action = menu->addAction("Add volume...");
                connections_.emplace_back(QObject::connect(
                        action, &QAction::triggered,
                        [action, this]()
                        {
                                add_volume(action_name(*action));
                        }));
        }
}

Actions::~Actions()
{
        connections_.clear();
        worker_threads_->terminate_all();
}

void Actions::set_progresses()
{
        worker_threads_->set_progresses();
}

void Actions::save_image(const std::string& action, const long long slice) const
{
        std::optional<Pixels::Images> images = pixels_->slice(slice);
        if (!images)
        {
                message_warning("Image is not yet available");
                return;
        }
        if (images->size.size() != 2)
        {
                message_warning("Error 2D image dimension " + to_string(images->size.size()));
                return;
        }

        worker_threads_->terminate_and_start(
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
        std::optional<Pixels::Images> images = pixels_->pixels();
        if (!images)
        {
                message_warning("Image is not yet available");
                return;
        }

        worker_threads_->terminate_and_start(
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
        std::optional<Pixels::Images> images = pixels_->pixels();
        if (!images)
        {
                message_warning("Image is not yet available");
                return;
        }

        worker_threads_->terminate_and_start(
                ADD_THREAD_ID, action,
                [&]()
                {
                        return painter_window::add_volume(
                                images->size, images->rgb.color_format, std::move(images->rgb.pixels),
                                images->rgba.color_format, std::move(images->rgba.pixels));
                });
}
}
