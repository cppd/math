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

#include "objects.h"

#include <array>
#include <atomic>
#include <string>

namespace ns::painter
{
template <std::size_t N>
struct PainterNotifier
{
protected:
        virtual ~PainterNotifier() = default;

public:
        virtual void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N>& pixel) = 0;
        virtual void painter_pixel_after(
                unsigned thread_number,
                const std::array<int_least16_t, N>& pixel,
                const Color& c,
                float coverage) = 0;
        virtual void painter_error_message(const std::string& msg) = 0;
};

template <std::size_t N, typename T>
void paint(
        PainterNotifier<N - 1>* painter_notifier,
        int samples_per_pixel,
        const Scene<N, T>& scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
}
