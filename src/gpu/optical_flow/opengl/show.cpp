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

#if defined(OPENGL_FOUND)

#include "show.h"

#include "compute.h"
#include "shader_source.h"

#include "com/container.h"
#include "com/error.h"
#include "com/matrix.h"
#include "com/matrix_alg.h"
#include "com/time.h"
#include "com/type/limit.h"
#include "com/vec.h"
#include "gpu/optical_flow/com/show.h"
#include "graphics/opengl/buffers.h"
//#include "graphics/opengl/capabilities.h"
#include "graphics/opengl/shader.h"

#include <optional>
#include <vector>

// Интервал ожидания для расчёта потока не для каждого кадра
// constexpr double COMPUTE_INTERVAL_SECONDS = 1.0 / 10;

namespace gpu_opengl
{
namespace
{
class ShaderMemory final
{
        static constexpr int POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int DATA_BINDING = 2;

        const opengl::Buffer* m_points = nullptr;
        const opengl::Buffer* m_points_flow = nullptr;
        opengl::Buffer m_buffer;

        struct Data
        {
                mat4f matrix;
        };

public:
        ShaderMemory() : m_buffer(sizeof(Data), GL_MAP_WRITE_BIT)
        {
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
                opengl::map_and_write_to_buffer(m_buffer, offsetof(Data, matrix), m);
        }

        void set_points(const opengl::Buffer& buffer)
        {
                m_points = &buffer;
        }

        void set_points_flow(const opengl::Buffer& buffer)
        {
                m_points_flow = &buffer;
        }

        void bind() const
        {
                ASSERT(m_points && m_points_flow);

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_BINDING, *m_points);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_FLOW_BINDING, *m_points_flow);
                glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BINDING, m_buffer);
        }
};

class Impl final : public OpticalFlowShow
{
        std::optional<opengl::GraphicsProgram> m_draw_prog;
        std::optional<opengl::GraphicsProgram> m_draw_prog_debug;

        std::optional<opengl::Buffer> m_top_points;
        std::optional<opengl::Buffer> m_top_points_flow;

        ShaderMemory m_shader_memory;

        std::unique_ptr<gpu_opengl::OpticalFlowCompute> m_optical_flow;

        int m_top_point_count;

        bool m_flow_computed = false;
        double m_last_time = limits<double>::lowest();

        int m_x, m_y, m_width, m_height;

        void draw_flow_lines()
        {
                m_shader_memory.bind();

                m_draw_prog->draw_arrays(GL_POINTS, 0, m_top_point_count * 2);
                m_draw_prog->draw_arrays(GL_LINES, 0, m_top_point_count * 2);
        }

        void reset() override
        {
                if (m_top_point_count == 0)
                {
                        return;
                }

                m_last_time = limits<double>::lowest();
                m_flow_computed = false;
                m_optical_flow->reset();
        }

        void draw() override
        {
                if (m_top_point_count == 0)
                {
                        return;
                }

                glViewport(m_x, m_y, m_width, m_height);

                // opengl::GLEnableAndRestore<GL_SCISSOR_TEST> e;
                // glScissor(0, 0, m_width, m_height);

#if 0
                double current_time = time_in_seconds();
                if (current_time - m_last_time < COMPUTE_INTERVAL_SECONDS)
                {
                        if (m_flow_computed)
                        {
                                draw_flow_lines();
                        }
                        return;
                }
                m_last_time = current_time;
#endif
                m_optical_flow->exec();

#if 0
                // m_draw_prog_debug.set_uniform_handle("tex", m_optical_flow->image_pyramid_dx_texture());
                m_draw_prog_debug.set_uniform_handle("tex", m_optical_flow->image_pyramid_texture());
                m_draw_prog_debug.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
#else
                draw_flow_lines();
                m_flow_computed = true;
#endif
        }

public:
        Impl(const opengl::Texture& source, double window_ppi, int x, int y, int width, int height)
                : m_x(x), m_y(y), m_width(width), m_height(height)
        {
                std::vector<vec2i> points;
                int point_count_x, point_count_y;
                create_top_level_optical_flow_points(
                        m_width, m_height, window_ppi, &point_count_x, &point_count_y, &points);

                m_top_point_count = points.size();

                if (m_top_point_count == 0)
                {
                        return;
                }

                m_draw_prog.emplace(
                        opengl::VertexShader(optical_flow_show_vert()),
                        opengl::FragmentShader(optical_flow_show_frag()));
                // m_draw_prog_debug.emplace(opengl::VertexShader(optical_flow_show_debug_vert()),
                //                          opengl::FragmentShader(optical_flow_show_debug_frag()));

                m_top_points.emplace(data_size(points), 0, points);
                m_top_points_flow.emplace(points.size() * sizeof(vec2f), 0);

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = m_width;
                double bottom = m_height;
                double top = 0;
                double near = 1;
                double far = -1;
                mat4 p = ortho_opengl<double>(left, right, bottom, top, near, far);
                mat4 t = translate(vec3(0.5, 0.5, 0));
                m_shader_memory.set_matrix(p * t);
                m_shader_memory.set_points(*m_top_points);
                m_shader_memory.set_points_flow(*m_top_points_flow);

                m_optical_flow = gpu_opengl::create_optical_flow_compute(
                        source, x, y, width, height, point_count_x, point_count_y, *m_top_points, *m_top_points_flow);
        }
};
}

std::unique_ptr<OpticalFlowShow> create_optical_flow_show(
        const opengl::Texture& source,
        double window_ppi,
        int x,
        int y,
        int width,
        int height)
{
        return std::make_unique<Impl>(source, window_ppi, x, y, width, height);
}
}

#endif
