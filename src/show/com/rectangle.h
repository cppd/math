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

#include "com/error.h"

template <typename T>
bool pointIsInsideRectangle(T x, T y, T x0, T y0, T x1, T y1)
{
        return x >= x0 && x < x1 && y >= y0 && y < y1;
}

inline bool windowPositionAndSize(
        bool two_windows,
        int width,
        int height,
        int frame,
        int* w1_x,
        int* w1_y,
        int* w1_w,
        int* w1_h,
        int* w2_x,
        int* w2_y,
        int* w2_w,
        int* w2_h)
{
        ASSERT(w1_x && w1_y && w1_w && w1_h && w2_x && w2_y && w2_w && w2_h);

        if (two_windows)
        {
                int w = (width - 3 * frame) / 2;
                int h = (height - 2 * frame);
                if (w > 0 && h > 0)
                {
                        *w1_x = frame;
                        *w1_y = frame;
                        *w1_w = w;
                        *w1_h = h;

                        *w2_x = width - frame - w;
                        *w2_y = frame;
                        *w2_w = w;
                        *w2_h = h;

                        return true;
                }
        }

        *w1_x = 0;
        *w1_y = 0;
        *w1_w = width;
        *w1_h = height;

        *w2_x = 0;
        *w2_y = 0;
        *w2_w = 0;
        *w2_h = 0;

        return false;
}
