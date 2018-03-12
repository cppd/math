/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include <array>
#include <atomic>
#include <string>

template <size_t N>
struct IPainterNotifier
{
protected:
        virtual ~IPainterNotifier()
        {
        }

public:
        virtual void painter_pixel_before(const std::array<int_least16_t, N>& pixel) noexcept = 0;
        virtual void painter_pixel_after(const std::array<int_least16_t, N>& pixel, const SrgbInteger& c) noexcept = 0;
        virtual void painter_error_message(const std::string& msg) noexcept = 0;
};

template <size_t N, typename T>
void paint(IPainterNotifier<N - 1>* painter_notifier, int samples_per_pixel, const PaintObjects<N, T>& paint_objects,
           Paintbrush<N - 1>* paintbrush, int thread_count, std::atomic_bool* stop) noexcept;
