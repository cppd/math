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

#include "opengl_ch_show.h"

#include "com/math.h"
#include "com/time.h"
#include "gpgpu/convex_hull/compute/opengl_ch_compute.h"
#include "gpgpu/convex_hull/show/objects/com.h"
#include "gpgpu/convex_hull/show/objects/opengl_shader.h"
#include "graphics/opengl/shader.h"

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

namespace
{
namespace impl
{
using namespace gpgpu_convex_hull_show_implementation;
using namespace gpgpu_convex_hull_show_opengl_implementation;
}

class Impl final : public gpgpu_opengl::ConvexHullShow
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::StorageBuffer m_points;
        double m_start_time;
        std::unique_ptr<gpgpu_opengl::ConvexHullCompute> m_convex_hull;
        impl::ShaderMemory m_shader_memory;

        void reset_timer() override
        {
                m_start_time = time_in_seconds();
        }

        void draw() override
        {
                int point_count = m_convex_hull->exec();

                float brightness = 0.5 + 0.5 * std::sin(impl::ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                m_shader_memory.bind();
                m_draw_prog.draw_arrays(GL_LINE_STRIP, 0, point_count);
        }

public:
        Impl(const opengl::TextureImage& objects, const mat4& matrix)
                : m_draw_prog(opengl::VertexShader(vertex_shader), opengl::FragmentShader(fragment_shader)),
                  m_points(impl::points_buffer_size(objects.height())),
                  m_start_time(time_in_seconds())
        {
                m_convex_hull = gpgpu_opengl::create_convex_hull_compute(objects, m_points);

                m_shader_memory.set_matrix(matrix);
                m_shader_memory.set_points(m_points);
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

namespace gpgpu_opengl
{
std::unique_ptr<ConvexHullShow> create_convex_hull_show(const opengl::TextureImage& objects, const mat4& matrix)
{
        return std::make_unique<Impl>(objects, matrix);
}
}
