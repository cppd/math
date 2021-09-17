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

#include "com/error.h"
#include "gui/application.h"

#include <exception>

int main(int argc, char** argv)
{
        try
        {
                try
                {
                        return ns::gui::run_application(argc, argv);
                }
                catch (const std::exception& e)
                {
                        ns::error_fatal(std::string("Error in the main function\n") + e.what());
                }
                catch (...)
                {
                        ns::error_fatal("Unknown error in the main function");
                }
        }
        catch (...)
        {
                ns::error_fatal("Exception in the main function exception handlers");
        }
}
