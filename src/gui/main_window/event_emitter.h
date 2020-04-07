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

#include <src/com/log.h>
#include <src/model/object_id.h>
#include <src/storage/events.h>
#include <src/view/interface.h>

#include <QObject>
#include <string>
#include <variant>

struct WindowEvent
{
        struct MessageError final
        {
                std::string msg;
                explicit MessageError(const std::string& msg) : msg(msg)
                {
                }
        };

        struct MessageErrorFatal final
        {
                std::string msg;
                explicit MessageErrorFatal(const std::string& msg) : msg(msg)
                {
                }
        };

        struct MessageInformation final
        {
                std::string msg;
                explicit MessageInformation(const std::string& msg) : msg(msg)
                {
                }
        };

        struct MessageWarning final
        {
                std::string msg;
                explicit MessageWarning(const std::string& msg) : msg(msg)
                {
                }
        };

        struct FileLoaded final
        {
                std::string file_name;
                size_t dimension;
                FileLoaded(const std::string& file_name, size_t dimension) : file_name(file_name), dimension(dimension)
                {
                }
        };

        using T = std::
                variant<std::monostate, FileLoaded, MessageError, MessageErrorFatal, MessageInformation, MessageWarning>;

        WindowEvent() : m_data(std::monostate())
        {
        }

        template <typename Type>
        WindowEvent(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

class WindowEventEmitter final : public QObject
{
        Q_OBJECT

signals:
        void window_event_signal(const WindowEvent&) const;

public:
        void message_error(const std::string& msg) const;
        void message_error_fatal(const std::string& msg) const;
        void message_information(const std::string& msg) const;
        void message_warning(const std::string& msg) const;
        void file_loaded(const std::string& file_name, size_t dimension) const;
};

//

struct WindowEventStorage
{
        struct LoadedObject final
        {
                ObjectId id;
                size_t dimension;
                LoadedObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct LoadedMesh final
        {
                ObjectId id;
                size_t dimension;
                explicit LoadedMesh(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct DeletedObject final
        {
                ObjectId id;
                size_t dimension;
                DeletedObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct DeletedAll final
        {
                size_t dimension;
                explicit DeletedAll(size_t dimension) : dimension(dimension)
                {
                }
        };

        using T = std::variant<std::monostate, DeletedAll, DeletedObject, LoadedMesh, LoadedObject>;

        WindowEventStorage() : m_data(std::monostate())
        {
        }

        template <typename Type>
        WindowEventStorage(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

class WindowEventEmitterStorage final : public QObject, public StorageEvents
{
        Q_OBJECT

signals:
        void window_event_signal(const WindowEventStorage&) const;

public:
        void loaded_object(ObjectId id, size_t dimension) const override;
        void loaded_mesh(ObjectId id, size_t dimension) const override;
        void deleted_object(ObjectId id, size_t dimension) const override;
        void deleted_all(size_t dimension) const override;
};

//

struct WindowEventView
{
        struct ErrorFatal final
        {
                std::string msg;
                explicit ErrorFatal(const std::string& msg) : msg(msg)
                {
                }
        };

        struct ObjectLoaded final
        {
                ObjectId id;
                explicit ObjectLoaded(ObjectId id) : id(id)
                {
                }
        };

        using T = std::variant<std::monostate, ErrorFatal, ObjectLoaded>;

        WindowEventView() : m_data(std::monostate())
        {
        }

        template <typename Type>
        WindowEventView(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

class WindowEventEmitterView final : public QObject, public view::Events
{
        Q_OBJECT

signals:
        void window_event_signal(const WindowEventView&) const;

public:
        void error_fatal(const std::string& msg) const override;
        void object_loaded(ObjectId id) const override;
};

//

struct WindowEventLog
{
        struct Log final
        {
                std::string msg;
                explicit Log(const std::string& msg) : msg(msg)
                {
                }
        };

        using T = std::variant<std::monostate, Log>;

        WindowEventLog() : m_data(std::monostate())
        {
        }

        template <typename Type>
        WindowEventLog(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

class WindowEventEmitterLog final : public QObject, public LogEvents
{
        Q_OBJECT

signals:
        void window_event_signal(const WindowEventLog&) const;

public:
        void log(const std::string& msg) const override;
};
