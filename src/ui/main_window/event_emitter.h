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

#include "com/error.h"
#include "com/log.h"
#include "com/variant.h"
#include "show/interface.h"
#include "ui/main_window/objects.h"

#include <QObject>

class DirectEvents
{
protected:
        virtual ~DirectEvents() = default;

public:
        virtual void direct_message_error(const std::string& msg) = 0;
        virtual void direct_message_error_fatal(const std::string& msg) = 0;
        virtual void direct_message_error_source(const std::string& msg, const std::string& src) = 0;
        virtual void direct_message_information(const std::string& msg) = 0;
        virtual void direct_message_warning(const std::string& msg) = 0;
        virtual void direct_object_loaded(int id) = 0;
        virtual void direct_mesh_loaded(ObjectId id) = 0;
        virtual void direct_file_loaded(
                const std::string& file_name,
                unsigned dimension,
                const std::unordered_set<ObjectId>& objects) = 0;
        virtual void direct_bound_cocone_loaded(double rho, double alpha) = 0;
        virtual void direct_log(const std::string& msg) = 0;
};

class WindowEventEmitter final : public QObject, public LogCallback, public ObjectsCallback, public ShowCallback
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
                struct object_loaded final
                {
                        const int id;
                        explicit object_loaded(int id_) : id(id_)
                        {
                        }
                };
                struct mesh_loaded final
                {
                        const ObjectId id;
                        explicit mesh_loaded(ObjectId id_) : id(id_)
                        {
                        }
                };
                struct file_loaded final
                {
                        const std::string file_name;
                        const unsigned dimension;
                        const std::unordered_set<ObjectId> objects;
                        file_loaded(
                                const std::string& file_name_,
                                unsigned dimension_,
                                const std::unordered_set<ObjectId>& objects_)
                                : file_name(file_name_), dimension(dimension_), objects(objects_)
                        {
                        }
                };
                struct bound_cocone_loaded final
                {
                        const double rho;
                        const double alpha;
                        bound_cocone_loaded(double rho_, double alpha_) : rho(rho_), alpha(alpha_)
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

                Variant<std::monostate,
                        object_loaded,
                        mesh_loaded,
                        bound_cocone_loaded,
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

        DirectEvents* m_direct_events;

        class Visitor
        {
                DirectEvents* m_f;

        public:
                explicit Visitor(DirectEvents* f) : m_f(f)
                {
                }

                void operator()(const std::monostate&)
                {
                }
                void operator()(const WindowEvent::message_error& d)
                {
                        m_f->direct_message_error(d.msg);
                }
                void operator()(const WindowEvent::message_error_fatal& d)
                {
                        m_f->direct_message_error_fatal(d.msg);
                }
                void operator()(const WindowEvent::message_error_source& d)
                {
                        m_f->direct_message_error_source(d.msg, d.src);
                }
                void operator()(const WindowEvent::message_information& d)
                {
                        m_f->direct_message_information(d.msg);
                }
                void operator()(const WindowEvent::message_warning& d)
                {
                        m_f->direct_message_warning(d.msg);
                }
                void operator()(const WindowEvent::object_loaded& d)
                {
                        m_f->direct_object_loaded(d.id);
                }
                void operator()(const WindowEvent::mesh_loaded& d)
                {
                        m_f->direct_mesh_loaded(d.id);
                }
                void operator()(const WindowEvent::file_loaded& d)
                {
                        m_f->direct_file_loaded(d.file_name, d.dimension, d.objects);
                }
                void operator()(const WindowEvent::bound_cocone_loaded& d)
                {
                        m_f->direct_bound_cocone_loaded(d.rho, d.alpha);
                }
                void operator()(const WindowEvent::log& d)
                {
                        m_f->direct_log(d.msg);
                }
        };

private slots:

        void slot_window_event(const WindowEvent& event)
        {
                visit(Visitor(m_direct_events), event.event);
        }

public:
        explicit WindowEventEmitter(DirectEvents* direct_events) : m_direct_events(direct_events)
        {
                ASSERT(m_direct_events);

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

        void message_warning(const std::string& msg) const override
        {
                emit_message<WindowEvent::message_warning>("Exception in emit message warning", msg);
        }

        void object_loaded(int id) const override
        {
                emit_message<WindowEvent::object_loaded>("Exception in emit object loaded", id);
        }

        void mesh_loaded(ObjectId id) const override
        {
                emit_message<WindowEvent::mesh_loaded>("Exception in emit mesh loaded", id);
        }

        void file_loaded(const std::string& file_name, unsigned dimension, const std::unordered_set<ObjectId>& objects)
                const override
        {
                emit_message<WindowEvent::file_loaded>("Exception in emit file loaded", file_name, dimension, objects);
        }

        void bound_cocone_loaded(double rho, double alpha) const override
        {
                emit_message<WindowEvent::bound_cocone_loaded>("Exception in emit BoundCocone loaded", rho, alpha);
        }

        void log(const std::string& msg) const override
        {
                emit_message<WindowEvent::log>("Exception in emit log", msg);
        }
};
