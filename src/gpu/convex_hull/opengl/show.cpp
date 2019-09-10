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

#include "show.h"

#include "compute.h"
#include "shader_source.h"
#include "show_memory.h"

#include "com/math.h"
#include "com/time.h"
#include "gpu/convex_hull/com/com.h"
#include "graphics/opengl/shader.h"

namespace gpu_opengl
{
namespace
{
class Impl final : public ConvexHullShow
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::StorageBuffer m_points;
        double m_start_time;
        std::unique_ptr<gpu_opengl::ConvexHullCompute> m_convex_hull;
        ConvexHullShaderMemory m_shader_memory;

        void reset_timer() override
        {
                m_start_time = time_in_seconds();
        }

        void draw() override
        {
                int point_count = m_convex_hull->exec();

                float brightness = 0.5 + 0.5 * std::sin(CONVEX_HULL_ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                m_shader_memory.bind();
                m_draw_prog.draw_arrays(GL_LINE_STRIP, 0, point_count);
        }

public:
        Impl(const opengl::Texture& objects, const mat4& matrix)
                : m_draw_prog(opengl::VertexShader(convex_hull_show_vert()), opengl::FragmentShader(convex_hull_show_frag())),
                  m_points(convex_hull_points_buffer_size(objects.height())),
                  m_start_time(time_in_seconds())
        {
                m_convex_hull = gpu_opengl::create_convex_hull_compute(objects, m_points);

                m_shader_memory.set_matrix(matrix);
                m_shader_memory.set_points(m_points);
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

std::unique_ptr<ConvexHullShow> create_convex_hull_show(const opengl::Texture& objects, const mat4& matrix)
{
        return std::make_unique<Impl>(objects, matrix);
}
}
