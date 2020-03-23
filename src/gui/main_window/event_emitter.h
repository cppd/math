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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/variant.h>
#include <src/storage/events.h>
#include <src/view/interface.h>

#include <QObject>
#include <variant>

class AllEvents
{
protected:
        virtual ~AllEvents() = default;

public:
        virtual void message_error(const std::string& msg) = 0;
        virtual void message_error_fatal(const std::string& msg) = 0;
        virtual void message_error_source(const std::string& msg, const std::string& src) = 0;
        virtual void message_information(const std::string& msg) = 0;
        virtual void message_warning(const std::string& msg) = 0;
        virtual void view_object_loaded(ObjectId id) = 0;

        virtual void loaded_object(ObjectId id, size_t dimension) = 0;
        virtual void loaded_mesh(ObjectId id, size_t dimension) = 0;
        virtual void deleted_object(ObjectId id, size_t dimension) = 0;
        virtual void deleted_all(size_t dimension) = 0;

        virtual void file_loaded(const std::string& file_name, size_t dimension) = 0;
        virtual void log(const std::string& msg) = 0;
};

class WindowEventEmitter final : public QObject, public LogEvents, public StorageEvents, public ViewEvents
{
        Q_OBJECT

private:
        struct WindowEvent final
        {
                struct message_error final
                {
                        const std::string msg;
                        explicit message_error(const std::string& msg_) : msg(msg_)
                        {
                        }
                };
                struct message_error_fatal final
                {
                        const std::string msg;
                        explicit message_error_fatal(const std::string& msg_) : msg(msg_)
                        {
                        }
                };
                struct message_error_source final
                {
                        const std::string msg;
                        const std::string src;
                        message_error_source(const std::string& msg_, const std::string& src_) : msg(msg_), src(src_)
                        {
                        }
                };
                struct message_information final
                {
                        const std::string msg;
                        explicit message_information(const std::string& msg_) : msg(msg_)
                        {
                        }
                };
                struct message_warning final
                {
                        const std::string msg;
                        explicit message_warning(const std::string& msg_) : msg(msg_)
                        {
                        }
                };
                struct view_object_loaded final
                {
                        const ObjectId id;
                        explicit view_object_loaded(ObjectId id_) : id(id_)
                        {
                        }
                };
                struct loaded_object final
                {
                        const ObjectId id;
                        const size_t dimension;
                        loaded_object(ObjectId id_, size_t dimension_) : id(id_), dimension(dimension_)
                        {
                        }
                };
                struct loaded_mesh final
                {
                        const ObjectId id;
                        const size_t dimension;
                        explicit loaded_mesh(ObjectId id_, size_t dimension_) : id(id_), dimension(dimension_)
                        {
                        }
                };
                struct deleted_object final
                {
                        const ObjectId id;
                        const size_t dimension;
                        deleted_object(ObjectId id_, size_t dimension_) : id(id_), dimension(dimension_)
                        {
                        }
                };
                struct deleted_all final
                {
                        const size_t dimension;
                        explicit deleted_all(size_t dimension_) : dimension(dimension_)
                        {
                        }
                };
                struct file_loaded final
                {
                        const std::string file_name;
                        const size_t dimension;
                        file_loaded(const std::string& file_name_, size_t dimension_)
                                : file_name(file_name_), dimension(dimension_)
                        {
                        }
                };
                struct log final
                {
                        const std::string msg;
                        explicit log(const std::string& msg_) : msg(msg_)
                        {
                        }
                };

                std::variant<
                        std::monostate,
                        view_object_loaded,
                        loaded_object,
                        loaded_mesh,
                        deleted_object,
                        deleted_all,
                        file_loaded,
                        message_error,
                        message_error_fatal,
                        message_error_source,
                        message_information,
                        message_warning,
                        log>
                        event;

                template <typename... Args>
                explicit WindowEvent(Args&&... args) : event(std::forward<Args>(args)...)
                {
                }
        };

signals:
        void window_event(const WindowEvent&) const;

private:
        template <typename T, typename... Args>
        void emit_message(const char* error_message, Args&&... args) const noexcept
        {
                try
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
                catch (...)
                {
                        error_fatal("Error emit message");
                }
        }

        AllEvents* m_all_events;

        class Visitor
        {
                AllEvents* m_f;

        public:
                explicit Visitor(AllEvents* f) : m_f(f)
                {
                }

                void operator()(const std::monostate&)
                {
                }
                void operator()(const WindowEvent::message_error& d)
                {
                        m_f->message_error(d.msg);
                }
                void operator()(const WindowEvent::message_error_fatal& d)
                {
                        m_f->message_error_fatal(d.msg);
                }
                void operator()(const WindowEvent::message_error_source& d)
                {
                        m_f->message_error_source(d.msg, d.src);
                }
                void operator()(const WindowEvent::message_information& d)
                {
                        m_f->message_information(d.msg);
                }
                void operator()(const WindowEvent::message_warning& d)
                {
                        m_f->message_warning(d.msg);
                }
                void operator()(const WindowEvent::view_object_loaded& d)
                {
                        m_f->view_object_loaded(d.id);
                }
                void operator()(const WindowEvent::loaded_object& d)
                {
                        m_f->loaded_object(d.id, d.dimension);
                }
                void operator()(const WindowEvent::loaded_mesh& d)
                {
                        m_f->loaded_mesh(d.id, d.dimension);
                }
                void operator()(const WindowEvent::deleted_object& d)
                {
                        m_f->deleted_object(d.id, d.dimension);
                }
                void operator()(const WindowEvent::deleted_all& d)
                {
                        m_f->deleted_all(d.dimension);
                }
                void operator()(const WindowEvent::file_loaded& d)
                {
                        m_f->file_loaded(d.file_name, d.dimension);
                }
                void operator()(const WindowEvent::log& d)
                {
                        m_f->log(d.msg);
                }
        };

private slots:

        void slot_window_event(const WindowEvent& event)
        {
                std::visit(Visitor(m_all_events), event.event);
        }

public:
        explicit WindowEventEmitter(AllEvents* all_events) : m_all_events(all_events)
        {
                ASSERT(m_all_events);

                qRegisterMetaType<WindowEvent>("WindowEvent");
                connect(this, SIGNAL(window_event(WindowEvent)), this, SLOT(slot_window_event(WindowEvent)),
                        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
        }

        void message_error(const std::string& msg) const
        {
                emit_message<WindowEvent::message_error>("Exception in emit message error", msg);
        }

        void message_error_fatal(const std::string& msg) const override
        {
                emit_message<WindowEvent::message_error_fatal>("Exception in emit message error fatal", msg);
        }

        void message_error_source(const std::string& msg, const std::string& src) const override
        {
                emit_message<WindowEvent::message_error_source>("Exception in emit message error source", msg, src);
        }

        void message_information(const std::string& msg) const
        {
                emit_message<WindowEvent::message_information>("Exception in emit message information", msg);
        }

        void message_warning(const std::string& msg) const
        {
                emit_message<WindowEvent::message_warning>("Exception in emit message warning", msg);
        }

        void view_object_loaded(ObjectId id) const override
        {
                emit_message<WindowEvent::view_object_loaded>("Exception in emit view object loaded", id);
        }

        void loaded_object(ObjectId id, size_t dimension) const override
        {
                emit_message<WindowEvent::loaded_object>("Exception in emit loaded object", id, dimension);
        }

        void loaded_mesh(ObjectId id, size_t dimension) const override
        {
                emit_message<WindowEvent::loaded_mesh>("Exception in emit loaded mesh", id, dimension);
        }

        void deleted_object(ObjectId id, size_t dimension) const override
        {
                emit_message<WindowEvent::deleted_object>("Exception in emit deleted object", id, dimension);
        }

        void deleted_all(size_t dimension) const override
        {
                emit_message<WindowEvent::deleted_all>("Exception in emit deleted_all", dimension);
        }

        void file_loaded(const std::string& file_name, size_t dimension) const
        {
                emit_message<WindowEvent::file_loaded>("Exception in emit file loaded", file_name, dimension);
        }

        void log(const std::string& msg) const override
        {
                emit_message<WindowEvent::log>("Exception in emit log", msg);
        }
};
