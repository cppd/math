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

#include "event.h"

#include "com/error.h"
#include "com/log.h"
#include "show/show.h"

#include <QObject>

class WindowEventEmitter final : public QObject, public ICallBack, public ILogCallback
{
        Q_OBJECT

signals:
        void window_event(const WindowEvent&) const;

public:
        void error_message(const std::string& msg) noexcept
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::error_message>, msg));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit error message: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit error message.");
                }
        }

        void error_fatal_message(const std::string& msg) noexcept override
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::error_fatal_message>, msg));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit error fatal message: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit error fatal message.");
                }
        }

        void error_source_message(const std::string& msg, const std::string& src) noexcept override
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::error_source_message>, msg, src));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit error source: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit error source.");
                }
        }

        void window_ready() noexcept override
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::window_ready>));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit window ready: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit window ready.");
                }
        }

        void object_loaded(int id) noexcept override
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::object_loaded>, id));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit object loaded: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit object loaded.");
                }
        }

        void file_loaded(const std::string& msg) noexcept
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::file_loaded>, msg));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit file loaded: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit file loaded.");
                }
        }

        void bound_cocone_loaded(double rho, double alpha) noexcept
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::bound_cocone_loaded>, rho, alpha));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit BOUND COCONE loaded: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit BOUND COCONE loaded.");
                }
        }

        void log(const std::string& msg) noexcept override
        {
                try
                {
                        emit window_event(WindowEvent(std::in_place_type<WindowEvent::log>, msg));
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in emit log: ") + e.what() + ".");
                }
                catch (...)
                {
                        error_fatal("exception in emit log.");
                }
        }
};
