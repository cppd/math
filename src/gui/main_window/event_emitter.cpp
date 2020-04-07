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

#include "event_emitter.h"

#include <src/com/error.h>

void WindowEventEmitter::message_error(const std::string& msg) const
{
        emit window_event_signal(WindowEvent::MessageError(msg));
}

void WindowEventEmitter::message_error_fatal(const std::string& msg) const
{
        emit window_event_signal(WindowEvent::MessageErrorFatal(msg));
}

void WindowEventEmitter::message_information(const std::string& msg) const
{
        emit window_event_signal(WindowEvent::MessageInformation(msg));
}

void WindowEventEmitter::message_warning(const std::string& msg) const
{
        emit window_event_signal(WindowEvent::MessageWarning(msg));
}

void WindowEventEmitter::file_loaded(const std::string& file_name, size_t dimension) const
{
        emit window_event_signal(WindowEvent::FileLoaded(file_name, dimension));
}

//

void WindowEventEmitterStorage::loaded_object(ObjectId id, size_t dimension) const
{
        emit window_event_signal(WindowEventStorage::LoadedObject(id, dimension));
}

void WindowEventEmitterStorage::loaded_mesh(ObjectId id, size_t dimension) const
{
        emit window_event_signal(WindowEventStorage::LoadedMesh(id, dimension));
}

void WindowEventEmitterStorage::deleted_object(ObjectId id, size_t dimension) const
{
        emit window_event_signal(WindowEventStorage::DeletedObject(id, dimension));
}

void WindowEventEmitterStorage::deleted_all(size_t dimension) const
{
        emit window_event_signal(WindowEventStorage::DeletedAll(dimension));
}

//

void WindowEventEmitterView::error_fatal(const std::string& msg) const
{
        emit window_event_signal(WindowEventView::ErrorFatal(msg));
}

void WindowEventEmitterView::object_loaded(ObjectId id) const
{
        emit window_event_signal(WindowEventView::ObjectLoaded(id));
}

//

void WindowEventEmitterLog::log(const std::string& msg) const
{
        emit window_event_signal(WindowEventLog::Log(msg));
}
