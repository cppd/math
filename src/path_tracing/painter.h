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

#include "objects.h"

#include "com/vec.h"

#include <atomic>
#include <string>

struct IPainterNotifier
{
protected:
        virtual ~IPainterNotifier()
        {
        }

public:
        virtual void painter_pixel_before(int x, int y) noexcept = 0;
        virtual void painter_pixel_after(int x, int y, unsigned char r, unsigned char g, unsigned char b) noexcept = 0;
        virtual void painter_error_message(const std::string& msg) noexcept = 0;
};

void paint(IPainterNotifier* painter_notifier, const PaintObjects* paint_objects, PixelSequence* pixel_sequence,
           unsigned thread_count, std::atomic_bool* stop, std::atomic_ullong* ray_count);
