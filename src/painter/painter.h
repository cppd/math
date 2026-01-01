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

#pragma once

#include "objects.h"

#include <src/image/image.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

namespace ns::painter
{
template <std::size_t N>
class Images final
{
        template <std::size_t>
        friend class ImagesReading;
        template <std::size_t>
        friend class ImagesWriting;

        mutable std::shared_mutex mutex_;
        image::Image<N> image_with_background_;
        image::Image<N> image_without_background_;

public:
        Images() = default;

        Images(const Images&) = delete;
        Images& operator=(const Images&) = delete;
        Images(Images&&) = delete;
        Images& operator=(Images&&) = delete;
};

template <std::size_t N>
class ImagesWriting final
{
        Images<N>* images_;
        std::unique_lock<std::shared_mutex> lock_;

public:
        explicit ImagesWriting(Images<N>* const images)
                : images_(images),
                  lock_(images_->mutex_)
        {
        }

        [[nodiscard]] image::Image<N>& image_with_background() const
        {
                return images_->image_with_background_;
        }

        [[nodiscard]] image::Image<N>& image_without_background() const
        {
                return images_->image_without_background_;
        }
};

template <std::size_t N>
class ImagesReading final
{
        const Images<N>* images_;
        std::shared_lock<std::shared_mutex> lock_;

public:
        explicit ImagesReading(const Images<N>* const images)
                : images_(images),
                  lock_(images_->mutex_)
        {
        }

        [[nodiscard]] const image::Image<N>& image_with_background() const
        {
                return images_->image_with_background_;
        }

        [[nodiscard]] const image::Image<N>& image_without_background() const
        {
                return images_->image_without_background_;
        }
};

template <std::size_t N>
class Notifier
{
protected:
        ~Notifier() = default;

public:
        virtual void thread_busy(unsigned thread_number, const std::array<int, N>& pixel) = 0;
        virtual void thread_free(unsigned thread_number) = 0;
        virtual void pixel_set(const std::array<int, N>& pixel, const numerical::Vector<3, float>& rgb) = 0;
        [[nodiscard]] virtual Images<N>* images(long long pass_number) = 0;
        virtual void pass_done(long long pass_number) = 0;

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

class Painter
{
public:
        virtual ~Painter() = default;

        virtual void wait() noexcept = 0;
        [[nodiscard]] virtual Statistics statistics() const = 0;
};

enum class Integrator
{
        BPT,
        PT
};

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Painter> create_painter(
        Integrator integrator,
        Notifier<N - 1>* notifier,
        int samples_per_pixel,
        std::optional<int> max_pass_count,
        const Scene<N, T, Color>* scene,
        int thread_count,
        bool flat_shading);
}
