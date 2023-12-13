/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "com/error.h"
#include "gui/application.h"

#include <atomic>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace
{
[[noreturn]] void terminate_write()
{
        if (std::current_exception())
        {
                try
                {
                        std::rethrow_exception(std::current_exception());
                }
                catch (const std::exception& e)
                {
                        try
                        {
                                ns::error_fatal(std::string("terminate called, exception: ") + e.what());
                        }
                        catch (...)
                        {
                                ns::error_fatal("terminate called, exception in exception handler");
                        }
                }
                catch (...)
                {
                        ns::error_fatal("terminate called, unknown exception");
                }
        }
        else
        {
                ns::error_fatal("terminate called, no exception");
        }
}

[[noreturn]] void terminate_handler()
{
        try
        {
                static std::atomic_int count = 0;
                ++count;
                if (count == 1)
                {
                        terminate_write();
                }
                if (count == 2)
                {
                        std::cerr << "terminate called, the second time\n";
                }
        }
        catch (...)
        {
        }
        std::abort();
}
}

int main(const int argc, char** const argv)
{
        std::set_terminate(terminate_handler);

        return ns::gui::run_application(argc, argv);
}
