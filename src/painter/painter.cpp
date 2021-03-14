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

#include "painter.h"

#include "painter/pixels.h"
#include "painter/sampler.h"
#include "painter/trace.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>

#include <random>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T>
class PixelData final
{
        std::atomic_bool* const m_stop;
        std::atomic_bool m_stop_painting = false;

        const Projector<N, T>& m_projector;
        Sampler<N - 1, T> m_sampler;
        Pixels<N - 1> m_pixels;
        PainterNotifier<N - 1>* const m_notifier;
        Paintbrush<N - 1>* const m_paintbrush;

public:
        PixelData(
                const Projector<N, T>& projector,
                int samples_per_pixel,
                PainterNotifier<N - 1>* painter_notifier,
                Paintbrush<N - 1>* paintbrush,
                std::atomic_bool* stop)
                : m_stop(stop),
                  m_projector(projector),
                  m_sampler(samples_per_pixel),
                  m_pixels(projector.screen_size()),
                  m_notifier(painter_notifier),
                  m_paintbrush(paintbrush)
        {
                paintbrush->first_pass();
        }

        bool stop()
        {
                if (*m_stop)
                {
                        m_stop_painting = true;
                }
                return m_stop_painting;
        }

        void set_stop()
        {
                m_stop_painting = true;
        }

        const Projector<N, T>& projector() const
        {
                return m_projector;
        }

        Sampler<N - 1, T>& sampler()
        {
                return m_sampler;
        }

        Pixels<N - 1>& pixels()
        {
                return m_pixels;
        }

        PainterNotifier<N - 1>& notifier()
        {
                return *m_notifier;
        }

        Paintbrush<N - 1>& paintbrush()
        {
                return *m_paintbrush;
        }
};

template <std::size_t N, typename T>
void paint_pixels(unsigned thread_number, const PaintData<N, T>& paint_data, PixelData<N, T>* pixel_data)
{
        thread_local RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
        thread_local std::vector<Vector<N - 1, T>> samples;

        samples.clear();
        int ray_count = 0;

        while (true)
        {
                if (pixel_data->stop())
                {
                        return;
                }

                std::optional<std::array<int_least16_t, N - 1>> pixel =
                        pixel_data->paintbrush().next_pixel(ray_count, samples.size());
                if (!pixel)
                {
                        return;
                }

                pixel_data->notifier().painter_pixel_before(thread_number, *pixel);

                pixel_data->sampler().generate(&samples);
                ray_count = 0;

                Vector<N - 1, T> screen_point = to_vector<T>(*pixel);
                int hit_sample_count = 0;
                Color color(0);

                for (const Vector<N - 1, T>& sample_point : samples)
                {
                        constexpr int RECURSION_LEVEL = 0;
                        constexpr Color::DataType COLOR_LEVEL = 1;

                        Ray<N, T> ray = pixel_data->projector().ray(screen_point + sample_point);

                        std::optional<Color> sample_color =
                                trace_path(paint_data, &ray_count, random_engine, RECURSION_LEVEL, COLOR_LEVEL, ray);

                        if (sample_color)
                        {
                                color += *sample_color;
                                ++hit_sample_count;
                        }
                }

                PixelInfo info = pixel_data->pixels().add(*pixel, color, hit_sample_count, samples.size());

                pixel_data->notifier().painter_pixel_after(thread_number, *pixel, info.color, info.coverage);
        }
}

template <std::size_t N, typename T>
void prepare_next_pass(unsigned thread_number, PixelData<N, T>* pixel_data)
{
        if (thread_number != 0)
        {
                return;
        }
        if (!pixel_data->paintbrush().next_pass())
        {
                pixel_data->set_stop();
        }
        pixel_data->sampler().next_pass();
}

template <std::size_t N, typename T>
void work_thread(
        unsigned thread_number,
        ThreadBarrier* barrier,
        const PaintData<N, T>& paint_data,
        PixelData<N, T>* pixel_data)
{
        while (true)
        {
                try
                {
                        paint_pixels(thread_number, paint_data, pixel_data);
                        barrier->wait();
                }
                catch (const std::exception& e)
                {
                        pixel_data->set_stop();
                        pixel_data->notifier().painter_error_message(std::string("Painter error:\n") + e.what());
                        barrier->wait();
                }
                catch (...)
                {
                        pixel_data->set_stop();
                        pixel_data->notifier().painter_error_message("Unknown painter error");
                        barrier->wait();
                }

                if (pixel_data->stop())
                {
                        return;
                }

                try
                {
                        prepare_next_pass<N, T>(thread_number, pixel_data);
                        barrier->wait();
                }
                catch (const std::exception& e)
                {
                        pixel_data->set_stop();
                        pixel_data->notifier().painter_error_message(std::string("Painter error:\n") + e.what());
                        barrier->wait();
                }
                catch (...)
                {
                        pixel_data->set_stop();
                        pixel_data->notifier().painter_error_message("Unknown painter error");
                        barrier->wait();
                }

                if (pixel_data->stop())
                {
                        return;
                }
        }
}

template <std::size_t N, typename T>
void paint_threads(int thread_count, const PaintData<N, T>& paint_data, PixelData<N, T>* pixel_data)
{
        ThreadBarrier barrier(thread_count);
        std::vector<std::thread> threads(thread_count);

        const auto thread_function = [&](unsigned thread_number) noexcept
        {
                try
                {
                        work_thread(thread_number, &barrier, paint_data, pixel_data);
                }
                catch (...)
                {
                        error_fatal("Exception in painter function");
                }
        };

        for (unsigned i = 0; i < threads.size(); ++i)
        {
                threads[i] = std::thread(thread_function, i);
        }

        for (std::thread& t : threads)
        {
                t.join();
        }
}
}

// Без выдачи исключений. Про проблемы сообщать через painter_notifier.
template <std::size_t N, typename T>
void paint(
        PainterNotifier<N - 1>* painter_notifier,
        int samples_per_pixel,
        const Scene<N, T>& scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept
{
        try
        {
                try
                {
                        if (!painter_notifier || !paintbrush || !stop)
                        {
                                error("Painter parameters not specified");
                        }
                        if (thread_count < 1)
                        {
                                error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
                        }
                        if (paintbrush->screen_size() != scene.projector().screen_size())
                        {
                                error("Painter paintbrush size (" + to_string(paintbrush->screen_size())
                                      + ") are not equal to the projector size ("
                                      + to_string(scene.projector().screen_size()) + ")");
                        }

                        const PaintData paint_data(scene, smooth_normal);

                        PixelData<N, T> pixel_data(
                                scene.projector(), samples_per_pixel, painter_notifier, paintbrush, stop);

                        paint_threads(thread_count, paint_data, &pixel_data);
                }
                catch (const std::exception& e)
                {
                        painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        painter_notifier->painter_error_message("Unknown painter error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter exception handlers");
        }
}

template void paint(
        PainterNotifier<2>* painter_notifier,
        int samples_per_pixel,
        const Scene<3, float>& scene,
        Paintbrush<2>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<3>* painter_notifier,
        int samples_per_pixel,
        const Scene<4, float>& scene,
        Paintbrush<3>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<4>* painter_notifier,
        int samples_per_pixel,
        const Scene<5, float>& scene,
        Paintbrush<4>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<5>* painter_notifier,
        int samples_per_pixel,
        const Scene<6, float>& scene,
        Paintbrush<5>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;

template void paint(
        PainterNotifier<2>* painter_notifier,
        int samples_per_pixel,
        const Scene<3, double>& scene,
        Paintbrush<2>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<3>* painter_notifier,
        int samples_per_pixel,
        const Scene<4, double>& scene,
        Paintbrush<3>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<4>* painter_notifier,
        int samples_per_pixel,
        const Scene<5, double>& scene,
        Paintbrush<4>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<5>* painter_notifier,
        int samples_per_pixel,
        const Scene<6, double>& scene,
        Paintbrush<5>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
}
