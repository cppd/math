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

#pragma once

#include "objects.h"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace ns::painter
{
template <std::size_t N>
struct Notifier
{
protected:
        virtual ~Notifier() = default;

public:
        virtual void thread_busy(unsigned thread_number, const std::array<int, N>& pixel) = 0;
        virtual void thread_free(unsigned thread_number) = 0;
        virtual void pixel_set(const std::array<int, N>& pixel, const Color& color, Color::DataType alpha) = 0;
        virtual void pass_done() = 0;

        virtual void error_message(const std::string& msg) = 0;
};

struct Statistics final
{
        long long pass_number;
        double pass_progress;
        long long pixel_count;
        long long ray_count;
        long long sample_count;
        double previous_pass_duration;

        Statistics() noexcept
        {
        }
};

template <std::size_t N, typename T>
struct Painter
{
        virtual ~Painter() = default;

        virtual void wait() noexcept = 0;
        virtual Statistics statistics() const = 0;
};

template <std::size_t N, typename T>
std::unique_ptr<Painter<N, T>> create_painter(
        Notifier<N - 1>* notifier,
        int samples_per_pixel,
        std::optional<int> max_pass_count,
        std::shared_ptr<const Scene<N, T>> scene,
        int thread_count,
        bool smooth_normal);
}
