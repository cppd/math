/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/error.h>
#include <src/numerical/region.h>

inline bool window_position_and_size(
        bool two_windows,
        int width,
        int height,
        int frame,
        Region<2, int>* window_1,
        Region<2, int>* window_2)
{
        ASSERT(window_1);
        ASSERT(window_2);

        if (two_windows)
        {
                int w = (width - 3 * frame) / 2;
                int h = (height - 2 * frame);
                if (w > 0 && h > 0)
                {
                        *window_1 = Region<2, int>(frame, frame, w, h);
                        *window_2 = Region<2, int>(width - frame - w, frame, w, h);
                        return true;
                }
        }

        *window_1 = Region<2, int>(0, 0, width, height);
        *window_2 = Region<2, int>(0, 0, 0, 0);

        return false;
}
