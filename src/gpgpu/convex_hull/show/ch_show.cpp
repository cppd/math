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

#include "ch_show.h"

#include "com/error.h"
#include "com/math.h"
#include "com/time.h"
#include "gpgpu/convex_hull/compute/ch_gl2d.h"

// clang-format off
constexpr const char vertex_shader[]
{
#include "ch_show.vert.str"
};
constexpr const char fragment_shader[]
{
#include "ch_show.frag.str"
};
// clang-format on

// rad / ms
constexpr double ANGULAR_FREQUENCY = TWO_PI<double> * 5;

class ConvexHull2DShow::Impl final
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::ShaderStorageBuffer m_points;
        double m_start_time;
        std::unique_ptr<ConvexHullGL2D> m_convex_hull;

public:
        Impl(const opengl::TextureR32I& objects, const mat4& matrix)
                : m_draw_prog(opengl::VertexShader(vertex_shader), opengl::FragmentShader(fragment_shader)),
                  m_start_time(time_in_seconds())
        {
                m_points.create_dynamic_copy((2 * objects.texture().height()) * (2 * sizeof(GLfloat)));

                m_convex_hull = create_convex_hull_gl2d(objects, m_points);

                m_draw_prog.set_uniform_float("matrix", matrix);
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        void reset_timer()
        {
                m_start_time = time_in_seconds();
        }

        void draw()
        {
                int point_count = m_convex_hull->exec();

                float d = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_draw_prog.set_uniform("brightness", d);

                m_points.bind(0);
                m_draw_prog.draw_arrays(GL_LINE_LOOP, 0, point_count);
        }
};

ConvexHull2DShow::ConvexHull2DShow(const opengl::TextureR32I& objects, const mat4& matrix)
        : m_impl(std::make_unique<Impl>(objects, matrix))
{
}

ConvexHull2DShow::~ConvexHull2DShow() = default;

void ConvexHull2DShow::reset_timer()
{
        m_impl->reset_timer();
}

void ConvexHull2DShow::draw()
{
        m_impl->draw();
}
