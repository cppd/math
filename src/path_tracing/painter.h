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
#include "vec3.h"

#include <memory>
#include <string>

struct IPainterNotifier
{
protected:
        virtual ~IPainterNotifier()
        {
        }

public:
        virtual void painter_pixel_before(int x, int y) noexcept = 0;
        virtual void painter_pixel_after(int x, int y, const vec3& color) noexcept = 0;
        virtual void painter_error_message(const std::string& msg) noexcept = 0;
};

class IPainter
{
public:
        virtual ~IPainter() = default;
        virtual long long get_rays_count() const noexcept = 0;
};

std::unique_ptr<IPainter> create_painter(IPainterNotifier* painter_notifier, const std::vector<const GenericObject*>& objects,
                                         const std::vector<const LightSource*>& light_sources, const Projector& projector,
                                         PixelSequence* pixel_sequence, unsigned thread_count);
