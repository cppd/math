/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "window.h"

#include <src/com/error.h>
#include <src/numerical/region.h>

#include <optional>
#include <tuple>

namespace ns::view::com
{
namespace
{
void check(const numerical::Region<2, int>& window, const int width, const int height)
{
        if (!(window.x0() >= 0) || !(window.y0() >= 0) || !(window.width() > 0) || !(window.height() > 0)
            || !(window.x1() <= width) || !(window.y1() <= height))
        {
                error("Error window data");
        }
}
}

std::tuple<numerical::Region<2, int>, std::optional<numerical::Region<2, int>>> window_position_and_size(
        const bool two_windows,
        const int width,
        const int height,
        const int frame)
{
        if (two_windows)
        {
                const int w = (width - 3 * frame) / 2;
                const int h = (height - 2 * frame);
                if (w > 0 && h > 0)
                {
                        const numerical::Region<2, int> w0({frame, frame}, {w, h});
                        const numerical::Region<2, int> w1({width - frame - w, frame}, {w, h});
                        check(w0, width, height);
                        check(w1, width, height);
                        return {w0, w1};
                }
        }

        const numerical::Region<2, int> w0({0, 0}, {width, height});
        check(w0, width, height);
        return {w0, std::nullopt};
}
}
