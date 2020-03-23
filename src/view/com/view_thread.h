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

#include "event_queue.h"

#include "../interface.h"

#include <src/com/error.h>

#include <atomic>
#include <thread>

template <typename T>
class ViewThread final : public ViewObject
{
        std::thread::id m_thread_id = std::this_thread::get_id();
        EventQueue m_event_queue;
        std::thread m_thread;
        std::atomic_bool m_stop{false};
        std::atomic_bool m_started{false};

        class EventQueueSetView final
        {
                EventQueue* m_event_queue;

        public:
                EventQueueSetView(EventQueue* event_queue, View* view) : m_event_queue(event_queue)
                {
                        m_event_queue->set_view(view);
                }
                ~EventQueueSetView()
                {
                        m_event_queue->set_view(nullptr);
                }
                EventQueueSetView(const EventQueueSetView&) = delete;
                EventQueueSetView(EventQueueSetView&&) = delete;
                EventQueueSetView& operator=(const EventQueueSetView&) = delete;
                EventQueueSetView& operator=(EventQueueSetView&&) = delete;
        };

        static void add_to_event_queue(EventQueue* queue, const ViewCreateInfo& info)
        {
                View& q = *queue;

                q.set_ambient(info.ambient.value());
                q.set_diffuse(info.diffuse.value());
                q.set_specular(info.specular.value());
                q.set_background_color(info.background_color.value());
                q.set_default_color(info.default_color.value());
                q.set_wireframe_color(info.wireframe_color.value());
                q.set_clip_plane_color(info.clip_plane_color.value());
                q.set_normal_length(info.normal_length.value());
                q.set_normal_color_positive(info.normal_color_positive.value());
                q.set_normal_color_negative(info.normal_color_negative.value());
                q.set_default_ns(info.default_ns.value());
                q.show_smooth(info.with_smooth.value());
                q.show_wireframe(info.with_wireframe.value());
                q.show_shadow(info.with_shadow.value());
                q.show_fog(info.with_fog.value());
                q.show_fps(info.with_fps.value());
                q.show_pencil_sketch(info.with_pencil_sketch.value());
                q.show_dft(info.with_dft.value());
                q.set_dft_brightness(info.dft_brightness.value());
                q.set_dft_background_color(info.dft_background_color.value());
                q.set_dft_color(info.dft_color.value());
                q.show_materials(info.with_materials.value());
                q.show_convex_hull_2d(info.with_convex_hull.value());
                q.show_optical_flow(info.with_optical_flow.value());
                q.set_vertical_sync(info.vertical_sync.value());
                q.set_shadow_zoom(info.shadow_zoom.value());
                q.show_normals(info.with_normals.value());
        }

        void thread_function(ViewEvents* events, WindowID parent_window, double parent_window_ppi)
        {
                try
                {
                        try
                        {
                                try
                                {
                                        T view(&m_event_queue, events, parent_window, parent_window_ppi);

                                        EventQueueSetView e(&m_event_queue, &view);

                                        m_started = true;

                                        view.loop(&m_stop);

                                        if (!m_stop)
                                        {
                                                error("Thread ended without stop.");
                                        }
                                }
                                catch (...)
                                {
                                        m_started = true;
                                        throw;
                                }
                        }
                        catch (ErrorSourceException& e)
                        {
                                events->message_error_source(e.msg(), e.src());
                        }
                        catch (std::exception& e)
                        {
                                events->message_error_fatal(e.what());
                        }
                        catch (...)
                        {
                                events->message_error_fatal("Unknown Error. Thread ended.");
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in the view thread exception handlers");
                }
        }

        View& view() override
        {
                return m_event_queue;
        }

        void join_thread()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }

public:
        explicit ViewThread(const ViewCreateInfo& info)
        {
                try
                {
                        try
                        {
                                add_to_event_queue(&m_event_queue, info);

                                if (!info.events.value() || !(info.window_ppi.value() > 0))
                                {
                                        error("View create information is not complete");
                                }

                                m_thread = std::thread(
                                        &ViewThread::thread_function, this, info.events.value(), info.window.value(),
                                        info.window_ppi.value());
                        }
                        catch (std::bad_optional_access&)
                        {
                                error("View create information is not complete");
                        }

                        do
                        {
                                std::this_thread::yield();
                        } while (!m_started);
                }
                catch (...)
                {
                        join_thread();
                        throw;
                }
        }

        ~ViewThread() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                join_thread();
        }
};
