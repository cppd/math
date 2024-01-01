/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "dimension.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/settings/utility.h>

namespace ns::process::dimension_implementation
{
[[noreturn]] void dimension_not_supported_error(const unsigned dimension)
{
        error("Dimension " + to_string(dimension) + " is not supported, supported dimensions are "
              + to_string(settings::supported_dimensions()) + ".");
}
}
