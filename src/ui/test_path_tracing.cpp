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

#include "test_path_tracing.h"

#include "painter_window.h"

#include "path_tracing/painter.h"
#include "path_tracing/pixel_sequence.h"
#include "path_tracing/projector.h"

constexpr vec3 DEFAULT_COLOR = vec3(0, 0.1, 0.2);
constexpr vec3 DEFAULT_LIGHT_SOURCE_COLOR = vec3(1, 1, 1);
constexpr bool DEFAULT_IS_LIGHT_SOURCE = false;

namespace
{
class Test : public PaintObjects
{
        const std::vector<const GenericObject*> m_objects;
        const std::vector<const LightSource*> m_light_sources;
        std::unique_ptr<PerspectiveProjector> m_projector;
        std::unique_ptr<Paintbrush> m_pixel_sequence;
        SurfaceProperties m_default_surface_properties;

public:
        Test(int width, int height)
        {
                reset_projector_and_sequence(width, height);

                m_default_surface_properties.set_color(DEFAULT_COLOR);
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(DEFAULT_IS_LIGHT_SOURCE);
                m_default_surface_properties.set_light_source_color(DEFAULT_LIGHT_SOURCE_COLOR);
        }

        const std::vector<const GenericObject*>& get_objects() const override
        {
                return m_objects;
        }

        const std::vector<const LightSource*>& get_light_sources() const override
        {
                return m_light_sources;
        }

        const Projector& get_projector() const override
        {
                return *m_projector;
        }

        PixelSequence& get_pixel_sequence() override
        {
                return *m_pixel_sequence;
        }

        const SurfaceProperties& get_default_surface_properties() const override
        {
                return m_default_surface_properties;
        }

        void reset_projector_and_sequence(int width, int height)
        {
                m_projector = std::make_unique<PerspectiveProjector>(vec3(1, 0, 0), vec3(-1, 0, 0), vec3(0, 0, 1), 120, width,
                                                                     height, 5);

                m_pixel_sequence = std::make_unique<Paintbrush>(width, height, 10);
        }
};
}

void test_path_tracing(int width, int height, unsigned thread_count)
{
        /* для отладки и тестов тут можно просто new */
        Test* test = new Test(width, height);

        create_painter_window(test, thread_count);
}
