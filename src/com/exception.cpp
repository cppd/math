/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "message.h"
#include "thread.h"

#include <exception>

namespace ns
{
namespace
{
void exception_handler(const std::exception_ptr& ptr, const std::string& description) noexcept
{
        try
        {
                try
                {
                        std::rethrow_exception(ptr);
                }
                catch (const TerminateRequestException&)
                {
                }
                catch (const std::exception& e)
                {
                        std::string s = !description.empty() ? (description + ":\n") : std::string();
                        MESSAGE_ERROR(s + e.what());
                }
                catch (...)
                {
                        std::string s = !description.empty() ? (description + ":\n") : std::string();
                        MESSAGE_ERROR(s + "Unknown error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in exception handlers");
        }
}
}

void catch_all(const std::string& description, const std::function<void()>& f) noexcept
{
        try
        {
                f();
        }
        catch (...)
        {
                exception_handler(std::current_exception(), description);
        }
}
}
