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

#include "dft_show.h"

#include "com/mat_glm.h"
#include "com/math.h"
#include "dft/comp/dft_gl2d.h"
#include "graphics/objects.h"

#include <vector>

// clang-format off
constexpr const char dft_show_vertex_shader[]
{
#include "dft_show.vert.str"
};
constexpr const char dft_show_fragment_shader[]
{
#include "dft_show.frag.str"
};
// clang-format on

constexpr int GROUP_SIZE = 16;

namespace
{
struct Vertex
{
        float v1, v2; // Координаты вершины в пространстве.
        float t1, t2; // Координаты вершины в текстуре.
        Vertex()
        {
        }
        Vertex(float v1_, float v2_, float t1_, float t2_) : v1(v1_), v2(v2_), t1(t1_), t2(t2_)
        {
        }
};
}

class DFTShow::Impl final
{
        const int m_groups_x, m_groups_y;
        const bool m_source_sRGB;
        TextureRGBA32F m_image_texture;
        std::unique_ptr<IFourierGL2> m_gl_fft;
        VertexArray m_vertex_array;
        ArrayBuffer m_vertex_buffer;
        std::vector<Vertex> m_vertices;
        GraphicsProgram m_draw_prog;

public:
        Impl(int width, int height, int pos_x, int pos_y, const mat4& mtx, bool source_sRGB)
                : m_groups_x(get_group_count(width, GROUP_SIZE)),
                  m_groups_y(get_group_count(height, GROUP_SIZE)),
                  m_source_sRGB(source_sRGB),
                  m_image_texture(width, height),
                  m_gl_fft(create_fft_gl2d(width, height, m_image_texture)),
                  m_vertices(4),
                  m_draw_prog(VertexShader(dft_show_vertex_shader), FragmentShader(dft_show_fragment_shader))
        {
                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v1), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t1), sizeof(Vertex), true);

                m_draw_prog.set_uniform_handle("tex", m_image_texture.get_texture().get_texture_resident_handle());
                set_brightness(1);

                int x_start = pos_x;
                int x_end = pos_x + width;
                int y_start = pos_y;
                int y_end = pos_y + height;

                vec4 pos00 = mtx * vec4(x_start, y_start, 0, 1);
                vec4 pos10 = mtx * vec4(x_end, y_start, 0, 1);
                vec4 pos01 = mtx * vec4(x_start, y_end, 0, 1);
                vec4 pos11 = mtx * vec4(x_end, y_end, 0, 1);

                // текстурный 0 находится внизу
                m_vertices[0] = Vertex(pos00[0], pos00[1], 0, 1);
                m_vertices[1] = Vertex(pos10[0], pos10[1], 1, 1);
                m_vertices[2] = Vertex(pos01[0], pos01[1], 0, 0);
                m_vertices[3] = Vertex(pos11[0], pos11[1], 1, 0);

                m_vertex_buffer.load_static_draw(m_vertices);
        }
        ~Impl()
        {
        }

        void set_brightness(float brightness)
        {
                m_draw_prog.set_uniform("brightness", brightness);
        }

        void copy_image()
        {
                m_image_texture.copy_texture_sub_image();
        }

        void draw()
        {
                m_gl_fft->exec(false, m_source_sRGB);

                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, m_vertices.size());
        }
};

DFTShow::DFTShow(int width, int height, int pos_x, int pos_y, const mat4& mtx, bool source_sRGB)
        : m_impl(std::make_unique<Impl>(width, height, pos_x, pos_y, mtx, source_sRGB))
{
}
DFTShow::~DFTShow() = default;

void DFTShow::set_brightness(float brightness)
{
        m_impl->set_brightness(brightness);
}

void DFTShow::copy_image()
{
        m_impl->copy_image();
}

void DFTShow::draw()
{
        m_impl->draw();
}
