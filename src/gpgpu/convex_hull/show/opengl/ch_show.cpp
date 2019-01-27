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

#include "ch_show.h"

#include "com/error.h"
#include "com/math.h"
#include "com/time.h"
#include "gpgpu/convex_hull/compute/ch_compute.h"
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

// rad / ms
constexpr double ANGULAR_FREQUENCY = TWO_PI<double> * 5;

namespace
{
class ShaderMemory
{
        static constexpr int DATA_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;

        opengl::UniformBuffer m_buffer;
        const opengl::StorageBuffer* m_points = nullptr;

        struct Data
        {
                Matrix<4, 4, float> matrix;
                float brightness;
        };

public:
        ShaderMemory() : m_buffer(sizeof(Data))
        {
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
                m_buffer.copy(offsetof(Data, matrix), m);
        }

        void set_brightness(float brightness) const
        {
                decltype(Data().brightness) b = brightness;
                m_buffer.copy(offsetof(Data, brightness), b);
        }

        void set_points(const opengl::StorageBuffer& points)
        {
                m_points = &points;
        }

        void bind() const
        {
                ASSERT(m_points);

                m_buffer.bind(DATA_BINDING);
                m_points->bind(POINTS_BINDING);
        }
};

int points_buffer_size(int height)
{
        // 2 линии точек + 1 точка, тип ivec2
        return (2 * height + 1) * (2 * sizeof(GLint));
}
}

namespace gpgpu_opengl
{
class ConvexHullShow::Impl final
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::StorageBuffer m_points;
        double m_start_time;
        std::unique_ptr<ConvexHullCompute> m_convex_hull;
        ShaderMemory m_shader_memory;

public:
        Impl(const opengl::TextureR32I& objects, const mat4& matrix)
                : m_draw_prog(opengl::VertexShader(vertex_shader), opengl::FragmentShader(fragment_shader)),
                  m_points(points_buffer_size(objects.texture().height())),
                  m_start_time(time_in_seconds())
        {
                m_convex_hull = create_convex_hull_compute(objects, m_points);

                m_shader_memory.set_matrix(matrix);
                m_shader_memory.set_points(m_points);
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

                float brightness = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (time_in_seconds() - m_start_time));
                m_shader_memory.set_brightness(brightness);

                m_shader_memory.bind();
                m_draw_prog.draw_arrays(GL_LINE_STRIP, 0, point_count);
        }
};

ConvexHullShow::ConvexHullShow(const opengl::TextureR32I& objects, const mat4& matrix)
        : m_impl(std::make_unique<Impl>(objects, matrix))
{
}

ConvexHullShow::~ConvexHullShow() = default;

void ConvexHullShow::reset_timer()
{
        m_impl->reset_timer();
}

void ConvexHullShow::draw()
{
        m_impl->draw();
}
}
