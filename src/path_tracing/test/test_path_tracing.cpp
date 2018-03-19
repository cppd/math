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

#include "test_path_tracing.h"

#include "com/file/file_sys.h"
#include "com/time.h"
#include "path_tracing/image/image.h"
#include "path_tracing/painter.h"
#include "path_tracing/scenes.h"
#include "path_tracing/shapes/test/test_mesh.h"
#include "path_tracing/visible_lights.h"
#include "path_tracing/visible_paintbrush.h"
#include "path_tracing/visible_projectors.h"

namespace
{
class Images : public IPainterNotifier<3>
{
        static constexpr const char beginning_of_file_name[] = "path_tracing_";

        std::vector<Image<2>> m_images;
        std::array<int, 3> m_size;

public:
        Images(const std::array<int, 3>& size) : m_size(size)
        {
                if (std::any_of(size.cbegin(), size.cend(), [](int v) { return v < 1; }))
                {
                        error("Error size " + to_string(size));
                }

                for (int i = 0; i < size[2]; ++i)
                {
                        m_images.emplace_back(std::array<int, 2>{{size[0], size[1]}});
                }
        }

        void painter_pixel_before(const std::array<int_least16_t, 3>&) noexcept override
        {
        }

        void painter_pixel_after(const std::array<int_least16_t, 3>& pixel, const SrgbInteger& c) noexcept override
        {
                try
                {
                        m_images[pixel[2]].set_pixel(std::array<int, 2>{{pixel[0], m_size[1] - 1 - pixel[1]}}, c);
                }
                catch (...)
                {
                        error_fatal("Exception in painter pixel after");
                }
        }

        void painter_error_message(const std::string& msg) noexcept override
        {
                LOG("Painter error message");
                LOG(msg);
        }

        void write_to_files(const std::string& dir) const
        {
                int w = std::floor(std::log10(m_images.size())) + 1;

                std::ostringstream oss;
                oss << std::setfill('0');

                for (unsigned i = 0; i < m_images.size(); ++i)
                {
                        oss.str("");
                        oss << beginning_of_file_name << std::setw(w) << i + 1;
                        m_images[i].write_to_file(dir + "/" + oss.str());
                }
        }
};

template <size_t N, typename T>
void test_path_tracing(int point_count, int one_dimension_screen_size)
{
        const int thread_count = hardware_concurrency();

        ProgressRatio progress(nullptr);

        std::shared_ptr<const Mesh<N, T>> mesh = simplex_mesh_of_random_sphere<N, T>(point_count, thread_count, &progress);

        LOG("Creating path tracing objects...");

        Vector<N, T> min, max;
        mesh->min_max(&min, &max);
        Vector<N, T> center = min + (max - min) / T(2);

        std::array<int, N - 1> screen_size;
        for (unsigned i = 0; i < screen_size.size(); ++i)
        {
                screen_size[i] = one_dimension_screen_size;
        }

        Vector<N, T> camera_position(center);
        camera_position[N - 1] = max[N - 1] + (max[N - 1] - min[N - 1]);

        Vector<N, T> camera_direction(0);
        camera_direction[N - 1] = -1;

        std::array<Vector<N, T>, N - 1> screen_axes;
        for (unsigned i = 0; i < N - 1; ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        screen_axes[i][n] = (i != n) ? 0 : 1;
                }
        }

        T view_width = T(1.2) * (max[0] - min[0]);

        Vector<N, T> light_position(max + (max - center));

        Color background_color = Color(SrgbInteger(50, 100, 150));
        Color default_color = Color(SrgbInteger(150, 170, 150));

        T diffuse = 1;

        int samples_per_pixel = 25;

        int paint_height = 2;
        int max_pass_count = 1;

        std::atomic_bool stop = false;

        Images images(screen_size);

        VisibleBarPaintbrush<N - 1> paintbrush(screen_size, paint_height, max_pass_count);

        std::unique_ptr<const Projector<N, T>> projector = std::make_unique<const VisibleParallelProjector<N, T>>(
                camera_position, camera_direction, screen_axes, view_width, screen_size);

        std::unique_ptr<const LightSource<N, T>> light_source =
                std::make_unique<const VisibleConstantLight<N, T>>(light_position, Color(1));

        std::unique_ptr<const PaintObjects<N, T>> paint_objects =
                one_object_scene(background_color, default_color, diffuse, std::move(projector), std::move(light_source), mesh);

        LOG("Painting...");
        double start_time = time_in_seconds();
        paint(&images, samples_per_pixel, *paint_objects, &paintbrush, thread_count, &stop);
        LOG("Painted, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        LOG("Writing screen images to files...");
        images.write_to_files(temp_directory());

        LOG("Done");
}
}

void test_path_tracing()
{
        test_path_tracing<4, double>(1000, 100);
}
