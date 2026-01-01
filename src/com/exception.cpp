/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "exception.h"

#include "error.h"
#include "message.h"

#include <exception>
#include <functional>
#include <string>

namespace ns
{
namespace
{
void message(const std::string& description, const char* const exception)
{
        std::string msg;
        if (!description.empty())
        {
                msg += description;
                msg += ":\n";
        }
        msg += exception;
        if (!msg.empty())
        {
                message_error(msg);
        }
        else
        {
                message_error("Exception without description and exception string");
        }
}
}

void catch_all(const std::string& description, const std::function<void()>& f) noexcept
{
        try
        {
                try
                {
                        f();
                }
                catch (const TerminateQuietlyException&)
                {
                }
                catch (const std::exception& e)
                {
                        message(description, e.what());
                }
                catch (...)
                {
                        message(description, "Unknown error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in catch all exception handlers");
        }
}
}
