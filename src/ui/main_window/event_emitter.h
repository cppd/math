/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "event.h"

#include "com/error.h"
#include "com/log.h"
#include "show/show.h"
#include "ui/main_window/objects.h"

#include <QObject>

class WindowEventEmitter final : public QObject, public ILogCallback, public IObjectsCallback, public IShowCallback
{
        Q_OBJECT

signals:
        void window_event(const WindowEvent&) const;

private:
        template <typename T, typename... Args>
        void emit_message(const char* error_message, Args&&... args) const noexcept
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<T>, std::forward<Args>(args)...));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string(error_message) + ": " + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal(std::string(error_message) + ".");
                }
        }

public:
        void message_error(const std::string& msg) const noexcept
        {
                emit_message<WindowEvent::message_error>("Exception in emit message error", msg);
        }

        void message_error_fatal(const std::string& msg) const noexcept override
        {
                emit_message<WindowEvent::message_error_fatal>("Exception in emit message error fatal", msg);
        }

        void message_error_source(const std::string& msg, const std::string& src) const noexcept override
        {
                emit_message<WindowEvent::message_error_source>("Exception in emit message error source", msg, src);
        }

        void message_information(const std::string& msg) const noexcept
        {
                emit_message<WindowEvent::message_information>("Exception in emit message information", msg);
        }

        void message_warning(const std::string& msg) const noexcept override
        {
                emit_message<WindowEvent::message_warning>("Exception in emit message warning", msg);
        }

        void object_loaded(int id) const noexcept override
        {
                emit_message<WindowEvent::loaded_object>("Exception in emit object loaded", id);
        }

        void mesh_loaded(ObjectId id) const noexcept override
        {
                emit_message<WindowEvent::loaded_mesh>("Exception in emit mesh loaded", id);
        }

        void file_loaded(const std::string& msg, unsigned dimension, const std::unordered_set<ObjectId>& objects) const
                noexcept override
        {
                emit_message<WindowEvent::loaded_file>("Exception in emit file loaded", msg, dimension, objects);
        }

        void bound_cocone_loaded(double rho, double alpha) const noexcept override
        {
                emit_message<WindowEvent::loaded_bound_cocone>("Exception in emit BoundCocone loaded", rho, alpha);
        }

        void log(const std::string& msg) const noexcept override
        {
                emit_message<WindowEvent::write_to_log>("Exception in emit log", msg);
        }
};
