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

#include "canvas.h"

#include "com/frequency.h"
#include "gpu_2d/opengl/convex_hull/convex_hull_2d.h"
#include "gpu_2d/opengl/dft/show/dft_show.h"
#include "gpu_2d/opengl/optical_flow/optical_flow.h"
#include "gpu_2d/opengl/pencil/pencil.h"
#include "gpu_2d/opengl/text/text.h"

namespace
{
class FPSText
{
        static constexpr double FPS_INTERVAL_LENGTH = 1;
        static constexpr int FPS_SAMPLE_COUNT = 10;

        Text m_text;
        Frequency m_fps;
        std::vector<std::string> m_fps_text;

public:
        FPSText(const char* fps_text, int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
                : m_text(size, step_y, start_x, start_y, color, matrix),
                  m_fps(FPS_INTERVAL_LENGTH, FPS_SAMPLE_COUNT),
                  m_fps_text({fps_text, ""})
        {
        }

        void set_color(const Color& color) const
        {
                m_text.set_color(color);
        }

        void set_matrix(const mat4& matrix) const
        {
                m_text.set_matrix(matrix);
        }

        void draw()
        {
                m_fps_text[1] = to_string(std::lround(m_fps.calculate()));
                m_text.draw(m_fps_text);
        }
};

class Canvas final : public OpenGLCanvas
{
        std::unique_ptr<FPSText> m_fps_text;
        std::unique_ptr<DFTShow> m_dft_show;
        std::unique_ptr<ConvexHull2D> m_convex_hull;
        std::unique_ptr<OpticalFlow> m_optical_flow;
        std::unique_ptr<PencilEffect> m_pencil_effect;

        int m_window_width;
        int m_window_height;

        bool m_fps_text_active = true;
        bool m_pencil_effect_active = false;
        bool m_dft_show_active = false;
        bool m_convex_hull_active = false;
        bool m_optical_flow_active = false;

        double m_dft_show_brightness = 1;
        Color m_dft_show_background_color = Color(0);
        Color m_dft_show_color = Color(1);

        Color m_fps_text_color = Color(1);

        void set_fps_text_color(const Color& c) override
        {
                m_fps_text_color = c;
                if (m_fps_text)
                {
                        m_fps_text->set_color(m_fps_text_color);
                }
        }

        void set_fps_text_active(bool v) override
        {
                m_fps_text_active = v;
        }

        void set_pencil_effect_active(bool v) override
        {
                m_pencil_effect_active = v;
        }

        bool pencil_effect_active() override
        {
                return m_pencil_effect_active;
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
                            int dft_dst_x, int dft_dst_y, bool frame_buffer_is_srgb, const char* fps_text, int text_size,
                            int text_step_y, int text_start_x, int text_start_y) override;

        void draw() override;
};

void Canvas::create_objects(int window_width, int window_height, const mat4& matrix, const opengl::TextureRGBA32F& color_texture,
                            bool color_texture_is_srgb, const opengl::TextureR32I& objects, int draw_width, int draw_height,
                            int dft_dst_x, int dft_dst_y, bool frame_buffer_is_srgb, const char* fps_text, int text_size,
                            int text_step_y, int text_start_x, int text_start_y)
{
        m_window_width = window_width;
        m_window_height = window_height;

        m_pencil_effect = std::make_unique<PencilEffect>(color_texture, color_texture_is_srgb, objects, matrix);

        m_dft_show = std::make_unique<DFTShow>(draw_width, draw_height, dft_dst_x, dft_dst_y, matrix, frame_buffer_is_srgb,
                                               m_dft_show_brightness, m_dft_show_background_color, m_dft_show_color);

        m_optical_flow = std::make_unique<OpticalFlow>(draw_width, draw_height, matrix);

        m_convex_hull = std::make_unique<ConvexHull2D>(objects, matrix);

        if (m_fps_text)
        {
                m_fps_text->set_matrix(matrix);
        }
        else
        {
                m_fps_text = std::make_unique<FPSText>(fps_text, text_size, text_step_y, text_start_x, text_start_y,
                                                       m_fps_text_color, matrix);
        }
}

void Canvas::draw()
{
        ASSERT(m_pencil_effect);
        ASSERT(m_dft_show);
        ASSERT(m_optical_flow);
        ASSERT(m_convex_hull);
        ASSERT(m_fps_text);

        glViewport(0, 0, m_window_width, m_window_height);

        if (m_pencil_effect_active)
        {
                // Рисование из цветного буфера в буфер экрана
                m_pencil_effect->draw();
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
        if (m_fps_text_active)
        {
                m_fps_text->draw();
        }
}
}

std::unique_ptr<OpenGLCanvas> create_opengl_canvas()
{
        return std::make_unique<Canvas>();
}
