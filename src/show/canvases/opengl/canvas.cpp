/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "canvas.h"

#include "gpgpu/convex_hull/show/opengl/ch_show.h"
#include "gpgpu/dft/show/dft_show.h"
#include "gpgpu/optical_flow/show/of_show.h"
#include "gpgpu/pencil_sketch/show/ps_show.h"
#include "text/opengl/text.h"

namespace
{
class Canvas final : public OpenGLCanvas
{
        int m_text_size;
        double m_window_ppi;

        std::unique_ptr<OpenGLText> m_text;
        std::unique_ptr<DFTShow> m_dft_show;
        std::unique_ptr<ConvexHullShow> m_convex_hull;
        std::unique_ptr<OpticalFlowShow> m_optical_flow;
        std::unique_ptr<PencilSketchShow> m_pencil_sketch;

        int m_window_width;
        int m_window_height;

        bool m_text_active = true;
        bool m_pencil_sketch_active = false;
        bool m_dft_show_active = false;
        bool m_convex_hull_active = false;
        bool m_optical_flow_active = false;

        double m_dft_show_brightness = 1;
        Color m_dft_show_background_color = Color(0);
        Color m_dft_show_color = Color(1);

        Color m_text_color = Color(1);

        void set_text_color(const Color& c) override
        {
                m_text_color = c;
                if (m_text)
                {
                        m_text->set_color(m_text_color);
                }
        }

        void set_text_active(bool v) override
        {
                m_text_active = v;
        }

        void set_pencil_sketch_active(bool v) override
        {
                m_pencil_sketch_active = v;
        }

        bool pencil_sketch_active() override
        {
                return m_pencil_sketch_active;
        }

        void set_dft_active(bool v) override
        {
                m_dft_show_active = v;
        }

        bool dft_active() override
        {
                return m_dft_show_active;
        }

        void set_dft_brightness(double v) override
        {
                m_dft_show_brightness = v;
                if (m_dft_show)
                {
                        m_dft_show->set_brightness(v);
                }
        }

        void set_dft_background_color(const Color& c) override
        {
                m_dft_show_background_color = c;
                if (m_dft_show)
                {
                        m_dft_show->set_background_color(c);
                }
        }

        void set_dft_color(const Color& c) override
        {
                m_dft_show_color = c;
                if (m_dft_show)
                {
                        m_dft_show->set_color(c);
                }
        }

        void set_convex_hull_active(bool v) override
        {
                m_convex_hull_active = v;
                if (m_convex_hull)
                {
                        m_convex_hull->reset_timer();
                }
        }

        void set_optical_flow_active(bool v) override
        {
                m_optical_flow_active = v;
                if (m_optical_flow)
                {
                        m_optical_flow->reset();
                }
        }

        void create_objects(int window_width, int window_height, const mat4& matrix, const opengl::TextureRGBA32F& color_texture,
                            bool color_texture_is_srgb, const opengl::TextureR32I& objects, int draw_width, int draw_height,
                            int dft_dst_x, int dft_dst_y, bool frame_buffer_is_srgb) override;

        void draw() override;
        void draw_text(int step_y, int x, int y, const std::vector<std::string>& text) override;

public:
        Canvas(int text_size, double window_ppi) : m_text_size(text_size), m_window_ppi(window_ppi)
        {
        }
};

void Canvas::create_objects(int window_width, int window_height, const mat4& matrix, const opengl::TextureRGBA32F& color_texture,
                            bool color_texture_is_srgb, const opengl::TextureR32I& objects, int draw_width, int draw_height,
                            int dft_dst_x, int dft_dst_y, bool frame_buffer_is_srgb)
{
        m_window_width = window_width;
        m_window_height = window_height;

        m_pencil_sketch = std::make_unique<PencilSketchShow>(color_texture, color_texture_is_srgb, objects, matrix);

        m_dft_show = std::make_unique<DFTShow>(draw_width, draw_height, dft_dst_x, dft_dst_y, matrix, frame_buffer_is_srgb,
                                               m_dft_show_brightness, m_dft_show_background_color, m_dft_show_color);

        m_optical_flow = std::make_unique<OpticalFlowShow>(draw_width, draw_height, m_window_ppi, matrix);

        m_convex_hull = std::make_unique<ConvexHullShow>(objects, matrix);

        if (m_text)
        {
                m_text->set_matrix(matrix);
        }
        else
        {
                m_text = std::make_unique<OpenGLText>(m_text_size, m_text_color, matrix);
        }
}

void Canvas::draw()
{
        ASSERT(m_pencil_sketch);
        ASSERT(m_dft_show);
        ASSERT(m_optical_flow);
        ASSERT(m_convex_hull);

        glViewport(0, 0, m_window_width, m_window_height);

        if (m_pencil_sketch_active)
        {
                // Рисование из цветного буфера в буфер экрана
                m_pencil_sketch->draw();
        }

        if (m_dft_show_active)
        {
                m_dft_show->take_image_from_framebuffer();
        }
        if (m_optical_flow_active)
        {
                m_optical_flow->take_image_from_framebuffer();
        }

        if (m_dft_show_active)
        {
                m_dft_show->draw();
        }
        if (m_optical_flow_active)
        {
                m_optical_flow->draw();
        }
        if (m_convex_hull_active)
        {
                m_convex_hull->draw();
        }
}

void Canvas::draw_text(int step_y, int x, int y, const std::vector<std::string>& text)
{
        ASSERT(m_text);

        if (m_text_active)
        {
                m_text->draw(step_y, x, y, text);
        }
}
}

std::unique_ptr<OpenGLCanvas> create_opengl_canvas(int text_size, double window_ppi)
{
        return std::make_unique<Canvas>(text_size, window_ppi);
}
