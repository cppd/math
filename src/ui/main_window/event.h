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
        struct loaded_object final
        {
                const int id;
                loaded_object(int id_) : id(id_)
                {
                }
        };
        struct loaded_file final
        {
                const std::string file_name;
                const unsigned dimension;
                loaded_file(const std::string& file_name_, unsigned dimension_) : file_name(file_name_), dimension(dimension_)
                {
                }
        };
        struct loaded_bound_cocone final
        {
                const double rho;
                const double alpha;
                loaded_bound_cocone(double rho_, double alpha_) : rho(rho_), alpha(alpha_)
                {
                }
        };
        struct write_to_log final
        {
                const std::string msg;
                write_to_log(const std::string& msg_) : msg(msg_)
                {
                }
        };

        enum class Type
        {
                LoadedBoundCocone,
                LoadedFile,
                LoadedObject,
                MessageError,
                MessageErrorFatal,
                MessageErrorSource,
                MessageInformation,
                MessageWarning,
                WriteToLog
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

        Type type() const
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
        static constexpr Type event_type(std::in_place_type_t<message_error>)
        {
                return Type::MessageError;
        }
        static constexpr Type event_type(std::in_place_type_t<message_error_fatal>)
        {
                return Type::MessageErrorFatal;
        }
        static constexpr Type event_type(std::in_place_type_t<message_error_source>)
        {
                return Type::MessageErrorSource;
        }
        static constexpr Type event_type(std::in_place_type_t<message_information>)
        {
                return Type::MessageInformation;
        }
        static constexpr Type event_type(std::in_place_type_t<message_warning>)
        {
                return Type::MessageWarning;
        }
        static constexpr Type event_type(std::in_place_type_t<loaded_object>)
        {
                return Type::LoadedObject;
        }
        static constexpr Type event_type(std::in_place_type_t<loaded_file>)
        {
                return Type::LoadedFile;
        }
        static constexpr Type event_type(std::in_place_type_t<loaded_bound_cocone>)
        {
                return Type::LoadedBoundCocone;
        }
        static constexpr Type event_type(std::in_place_type_t<write_to_log>)
        {
                return Type::WriteToLog;
        }

        Type m_type;

#if !defined(STD_VARIANT_NOT_FOUND)
        std::variant
#else
        SimpleVariant
#endif
                <std::monostate, loaded_object, loaded_bound_cocone, loaded_file, message_error, message_error_fatal,
                 message_error_source, message_information, message_warning, write_to_log>
                        m_data;
};
