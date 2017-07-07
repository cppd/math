/*
Copyright (C) 2017 Topological Manifold

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
#include <variant>

class WindowEvent final
{
public:
        struct error_message final
        {
                const std::string msg;
                error_message(const std::string& msg_) : msg(msg_)
                {
                }
        };
        struct window_ready final
        {
                window_ready()
                {
                }
        };
        struct program_ended final
        {
                const std::string msg;
                program_ended(const std::string& msg_) : msg(msg_)
                {
                }
        };
        struct error_src_message final
        {
                const std::string msg;
                const std::string src;
                error_src_message(const std::string& msg_, const std::string& src_) : msg(msg_), src(src_)
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

        enum class EventType
        {
                ERROR_MESSAGE,
                WINDOW_READY,
                PROGRAM_ENDED,
                ERROR_SRC_MESSAGE,
                OBJECT_LOADED,
                FILE_LOADED,
                BOUND_COCONE_LOADED
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

        EventType get_type() const
        {
                return m_type;
        }

        template <typename D>
        const D& get() const
        {
                // return m_data.get<D>();
                return std::get<D>(m_data);
        }

private:
        static constexpr EventType event_type(std::in_place_type_t<error_message>)
        {
                return EventType::ERROR_MESSAGE;
        }
        static constexpr EventType event_type(std::in_place_type_t<window_ready>)
        {
                return EventType::WINDOW_READY;
        }
        static constexpr EventType event_type(std::in_place_type_t<program_ended>)
        {
                return EventType::PROGRAM_ENDED;
        }
        static constexpr EventType event_type(std::in_place_type_t<error_src_message>)
        {
                return EventType::ERROR_SRC_MESSAGE;
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

        EventType m_type;

        std::variant<std::monostate, error_message, window_ready, program_ended, error_src_message, object_loaded, file_loaded,
                     bound_cocone_loaded>
                m_data;
};
