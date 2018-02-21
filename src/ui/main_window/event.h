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

#include <string>

#if !defined(STD_VARIANT_NOT_FOUND)
#include <variant>
#else
#include "com/simple_variant.h"
#endif

class WindowEvent final
{
public:
        struct message_error final
        {
                const std::string msg;
                message_error(const std::string& msg_) : msg(msg_)
                {
                }
        };
        struct message_error_fatal final
        {
                const std::string msg;
                message_error_fatal(const std::string& msg_) : msg(msg_)
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
                message_information(const std::string& msg_) : msg(msg_)
                {
                }
        };
        struct message_warning final
        {
                const std::string msg;
                message_warning(const std::string& msg_) : msg(msg_)
                {
                }
        };
        struct object_loaded final
        {
                const int id;
                object_loaded(int id_) : id(id_)
                {
                }
        };
        struct file_loaded final
        {
                const std::string file_name;
                file_loaded(const std::string& file_name_) : file_name(file_name_)
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
                log(const std::string& msg_) : msg(msg_)
                {
                }
        };

        enum class EventType
        {
                MESSAGE_ERROR,
                MESSAGE_ERROR_FATAL,
                MESSAGE_ERROR_SOURCE,
                MESSAGE_INFORMATION,
                MESSAGE_WARNING,
                OBJECT_LOADED,
                FILE_LOADED,
                BOUND_COCONE_LOADED,
                LOG
        };

        WindowEvent() = default;

#if 0
        template <typename T>
        WindowEvent(T&& v) : m_type(event_type(in_place<T>)), m_data(std::forward<T>(v))
        {
        }
#endif

        template <typename T, typename... Args>
        WindowEvent(std::in_place_type_t<T>, Args&&... args)
                : m_type(event_type(std::in_place_type<T>)), m_data(std::in_place_type<T>, std::forward<Args>(args)...)
        {
        }

        EventType type() const
        {
                return m_type;
        }

        template <typename D>
        const D& get() const
        {
#if !defined(STD_VARIANT_NOT_FOUND)
                return std::get<D>(m_data);
#else
                return m_data.get<D>();
#endif
        }

private:
        static constexpr EventType event_type(std::in_place_type_t<message_error>)
        {
                return EventType::MESSAGE_ERROR;
        }
        static constexpr EventType event_type(std::in_place_type_t<message_error_fatal>)
        {
                return EventType::MESSAGE_ERROR_FATAL;
        }
        static constexpr EventType event_type(std::in_place_type_t<message_error_source>)
        {
                return EventType::MESSAGE_ERROR_SOURCE;
        }
        static constexpr EventType event_type(std::in_place_type_t<message_information>)
        {
                return EventType::MESSAGE_INFORMATION;
        }
        static constexpr EventType event_type(std::in_place_type_t<message_warning>)
        {
                return EventType::MESSAGE_WARNING;
        }
        static constexpr EventType event_type(std::in_place_type_t<object_loaded>)
        {
                return EventType::OBJECT_LOADED;
        }
        static constexpr EventType event_type(std::in_place_type_t<file_loaded>)
        {
                return EventType::FILE_LOADED;
        }
        static constexpr EventType event_type(std::in_place_type_t<bound_cocone_loaded>)
        {
                return EventType::BOUND_COCONE_LOADED;
        }
        static constexpr EventType event_type(std::in_place_type_t<log>)
        {
                return EventType::LOG;
        }

        EventType m_type;

#if !defined(STD_VARIANT_NOT_FOUND)
        std::variant
#else
        SimpleVariant
#endif
                <std::monostate, message_error, message_error_fatal, message_error_source, message_information, message_warning,
                 object_loaded, file_loaded, bound_cocone_loaded, log>
                        m_data;
};
