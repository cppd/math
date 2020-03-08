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

#include "test_painter.h"

#include "../painter.h"
#include "../scenes/single_object.h"
#include "../shapes/mesh.h"
#include "../shapes/test/sphere_mesh.h"
#include "../visible_lights.h"
#include "../visible_paintbrush.h"
#include "../visible_projectors.h"

#include <src/com/names.h>
#include <src/com/time.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/gui/support/support.h>
#include <src/image/image.h>
#include <src/model/mesh_utility.h>
#include <src/utility/file/sys.h>
#include <src/utility/string/str.h>

constexpr Srgb8 BACKGROUND_COLOR(50, 100, 150);
constexpr Srgb8 DEFAULT_COLOR(150, 170, 150);

namespace
{
class Images : public PainterNotifier<3>
{
        static constexpr const char* BEGINNING_OF_FILE_NAME = "painter_";

        std::vector<Image<2>> m_images;
        std::array<int, 3> m_size;

public:
        explicit Images(const std::array<int, 3>& size) : m_size(size)
        {
                if (std::any_of(size.cbegin(), size.cend(), [](int v) { return v < 1; }))
                {
                        error("Error size " + to_string(size));
                }

                for (int i = 0; i < size[2]; ++i)
                {
                        m_images.emplace_back(std::array<int, 2>{size[0], size[1]});
                }
        }

        void painter_pixel_before(unsigned, const std::array<int_least16_t, 3>&) override
        {
        }

        void painter_pixel_after(unsigned, const std::array<int_least16_t, 3>& pixel, const Color& color) override
        {
                try
                {
                        m_images[pixel[2]].set_pixel(std::array<int, 2>{pixel[0], m_size[1] - 1 - pixel[1]}, color);
                }
                catch (...)
                {
                        error_fatal("Exception in painter pixel after");
                }
        }

        void painter_error_message(const std::string& msg) override
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
                        oss << BEGINNING_OF_FILE_NAME << std::setw(w) << i + 1;
                        m_images[i].write_to_file(dir + "/" + oss.str());
                }
        }
};

void check_application_instance()
{
        if (!QApplication::instance())
        {
                error("No application object for path tracing tests.\n"
                      "#include <QApplication>\n"
                      "int main(int argc, char* argv[])\n"
                      "{\n"
                      "    QApplication a(argc, argv);\n"
                      "    //\n"
                      "    return a.exec();\n"
                      "}\n");
        }
}

template <size_t N, typename T>
std::shared_ptr<const SpatialMeshModel<N, T>> sphere_mesh(int point_count, int thread_count, ProgressRatio* progress)
{
        LOG("Creating mesh...");
        std::shared_ptr<const SpatialMeshModel<N, T>> mesh =
                simplex_mesh_of_random_sphere<N, T>(point_count, thread_count, progress);

        return mesh;
}

template <size_t N, typename T>
std::shared_ptr<const SpatialMeshModel<N, T>> file_mesh(
        const std::string& file_name,
        int thread_count,
        ProgressRatio* progress)
{
        constexpr Matrix<N + 1, N + 1, T> matrix(1);

        LOG("Loading geometry from file...");
        std::unique_ptr<const mesh::Mesh<N>> mesh = mesh::load_geometry<N>(file_name, progress);

        LOG("Creating mesh...");
        std::shared_ptr<const SpatialMeshModel<N, T>> spatial_mesh =
                std::make_shared<const SpatialMeshModel<N, T>>(mesh.get(), matrix, thread_count, progress);

        return spatial_mesh;
}

template <size_t N, typename T>
void test_painter_file(
        int samples_per_pixel,
        int thread_count,
        std::unique_ptr<const PaintObjects<N, T>>&& paint_objects)
{
        constexpr int paint_height = 2;
        constexpr int max_pass_count = 1;
        constexpr bool smooth_normal = true;

        Images images(paint_objects->projector().screen_size());

        VisibleBarPaintbrush<N - 1> paintbrush(paint_objects->projector().screen_size(), paint_height, max_pass_count);

        std::atomic_bool stop = false;

        LOG("Painting...");
        double start_time = time_in_seconds();
        paint(&images, samples_per_pixel, *paint_objects, &paintbrush, thread_count, &stop, smooth_normal);
        LOG("Painted, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        LOG("Writing screen images to files...");
        images.write_to_files(temp_directory());

        LOG("Done");
}

template <size_t N, typename T>
void test_painter_window(
        int samples_per_pixel,
        int thread_count,
        std::unique_ptr<const PaintObjects<N, T>>&& paint_objects)
{
        constexpr bool smooth_normal = true;

        LOG("Window painting...");

        check_application_instance();

        std::string window_title = "Path Tracing In " + to_upper_first_letters(space_name(N));

        create_and_show_delete_on_close_window<PainterWindow<N, T>>(
                window_title, thread_count, samples_per_pixel, smooth_normal, std::move(paint_objects));
}

enum class PainterTestOutputType
{
        File,
        Window
};

template <PainterTestOutputType type, size_t N, typename T>
void test_painter(
        const std::shared_ptr<const SpatialMeshModel<N, T>>& mesh,
        int min_screen_size,
        int max_screen_size,
        int samples_per_pixel,
        int thread_count)
{
        Color::DataType diffuse = 1;

        std::unique_ptr<const PaintObjects<N, T>> paint_objects =
                single_object_scene(BACKGROUND_COLOR, DEFAULT_COLOR, diffuse, min_screen_size, max_screen_size, mesh);

        static_assert(type == PainterTestOutputType::File || type == PainterTestOutputType::Window);

        if constexpr (type == PainterTestOutputType::File)
        {
                test_painter_file(samples_per_pixel, thread_count, std::move(paint_objects));
        }

        if constexpr (type == PainterTestOutputType::Window)
        {
                test_painter_window(samples_per_pixel, thread_count, std::move(paint_objects));
        }
}

template <size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, int point_count, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::shared_ptr<const SpatialMeshModel<N, T>> mesh = sphere_mesh<N, T>(point_count, thread_count, &progress);

        test_painter<type>(mesh, min_screen_size, max_screen_size, samples_per_pixel, thread_count);
}

template <size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, const std::string& file_name, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::shared_ptr<const SpatialMeshModel<N, T>> mesh = file_mesh<N, T>(file_name, thread_count, &progress);

        test_painter<type>(mesh, min_screen_size, max_screen_size, samples_per_pixel, thread_count);
}
}

void test_painter_file()
{
        constexpr unsigned N = 4;
        int samples_per_pixel = 25;
        test_painter<N, double, PainterTestOutputType::File>(samples_per_pixel, 1000, 10, 100);
}

void test_painter_file(const std::string& file_name)
{
        constexpr unsigned N = 4;
        int samples_per_pixel = 25;
        test_painter<N, double, PainterTestOutputType::File>(samples_per_pixel, file_name, 10, 100);
}

void test_painter_window()
{
        constexpr unsigned N = 4;
        int samples_per_pixel = 25;
        test_painter<N, double, PainterTestOutputType::Window>(samples_per_pixel, 1000, 50, 500);
}

void test_painter_window(const std::string& file_name)
{
        constexpr unsigned N = 4;
        int samples_per_pixel = 25;
        test_painter<N, double, PainterTestOutputType::Window>(samples_per_pixel, file_name, 50, 500);
}
