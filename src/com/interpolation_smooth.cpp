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

#include "interpolation_smooth.h"

#include "enum.h"
#include "error.h"
#include "print.h"

#include <string_view>

namespace ns
{
std::string_view smooth_to_string(const Smooth smooth)
{
        switch (smooth)
        {
        case Smooth::N_0:
                return "N_0";
        case Smooth::N_1:
                return "N_1";
        case Smooth::N_2:
                return "N_2";
        case Smooth::N_3:
                return "N_3";
        case Smooth::N_4:
                return "N_4";
        }
        error("Unknown interpolation smooth type " + to_string(enum_to_int(smooth)));
}
}
