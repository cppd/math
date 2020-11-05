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
#include "../scenes/simple.h"
#include "../shapes/mesh.h"
#include "../shapes/test/sphere_mesh.h"
#include "../visible_lights.h"
#include "../visible_paintbrush.h"
#include "../visible_projectors.h"

#include <src/com/names.h>
#include <src/com/time.h>
#include <src/gui/com/support.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/image/color_image.h>
#include <src/model/mesh_utility.h>
#include <src/utility/file/path.h>
#include <src/utility/string/str.h>

#include <filesystem>

namespace painter
{
namespace
{
constexpr Srgb8 BACKGROUND_COLOR(50, 100, 150);
constexpr Srgb8 DEFAULT_COLOR(150, 170, 150);
constexpr Color::DataType DIFFUSE = 1;
constexpr Color::DataType LIGHTING_INTENSITY = 1;

class Images : public PainterNotifier<3>
{
        static constexpr const char* BEGINNING_OF_FILE_NAME = "painter_";

        std::vector<image::ColorImage<2>> m_images;
        std::array<int, 3> m_size;
        Color m_background_color;

public:
        Images(const std::array<int, 3>& size, const Color& background_color)
                : m_size(size), m_background_color(background_color)
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

        void painter_pixel_after(
                unsigned,
                const std::array<int_least16_t, 3>& pixel,
                const Color& color,
                float coverage) override
        {
                try
                {
                        m_images[pixel[2]].set_pixel(
                                std::array<int, 2>{pixel[0], m_size[1] - 1 - pixel[1]},
                                interpolation(m_background_color, color, coverage));
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

        void write_to_files() const
        {
                int w = std::floor(std::log10(m_images.size())) + 1;

                std::ostringstream oss;
                oss << std::setfill('0');

                for (unsigned i = 0; i < m_images.size(); ++i)
                {
                        oss.str("");
                        oss << BEGINNING_OF_FILE_NAME << std::setw(w) << i + 1;
                        m_images[i].write_to_file(std::filesystem::temp_directory_path() / path_from_utf8(oss.str()));
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
std::unique_ptr<const shapes::Mesh<N, T>> sphere_mesh(int point_count, ProgressRatio* progress)
{
        LOG("Creating mesh...");

        return shapes::simplex_mesh_of_random_sphere<N, T>(DEFAULT_COLOR, DIFFUSE, point_count, progress);
}

template <size_t N, typename T>
std::unique_ptr<const shapes::Mesh<N, T>> file_mesh(const std::string& file_name, ProgressRatio* progress)
{
        LOG("Loading geometry from file...");
        std::unique_ptr<const mesh::Mesh<N>> mesh = mesh::load<N>(file_name, progress);

        LOG("Creating mesh...");
        mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");
        {
                mesh::Writing writing(&mesh_object);
                writing.set_color(DEFAULT_COLOR);
                writing.set_diffuse(DIFFUSE);
        }

        std::vector<const mesh::MeshObject<N>*> meshes;
        meshes.push_back(&mesh_object);
        return std::make_unique<const shapes::Mesh<N, T>>(meshes, progress);
}

template <size_t N, typename T>
void test_painter_file(int samples_per_pixel, int thread_count, std::unique_ptr<const Scene<N, T>>&& scene)
{
        constexpr int paint_height = 2;
        constexpr int max_pass_count = 1;
        constexpr bool smooth_normal = true;

        Images images(scene->projector().screen_size(), scene->background_color());

        VisibleBarPaintbrush<N - 1> paintbrush(scene->projector().screen_size(), paint_height, max_pass_count);

        std::atomic_bool stop = false;

        LOG("Painting...");
        TimePoint start_time = time();
        paint(&images, samples_per_pixel, *scene, &paintbrush, thread_count, &stop, smooth_normal);
        LOG("Painted, " + to_string_fixed(duration_from(start_time), 5) + " s");

        LOG("Writing screen images to files...");
        images.write_to_files();

        LOG("Done");
}

template <size_t N, typename T>
void test_painter_window(int samples_per_pixel, int thread_count, std::unique_ptr<const Scene<N, T>>&& scene)
{
        constexpr bool smooth_normal = true;

        LOG("Window painting...");

        check_application_instance();

        std::string name = "Path Tracing In " + to_upper_first_letters(space_name(N));

        gui::create_painter_window(name, thread_count, samples_per_pixel, smooth_normal, std::move(scene));
}

enum class PainterTestOutputType
{
        File,
        Window
};

template <PainterTestOutputType type, size_t N, typename T>
void test_painter(
        std::unique_ptr<const Shape<N, T>>&& shape,
        int min_screen_size,
        int max_screen_size,
        int samples_per_pixel,
        int thread_count)
{
        std::unique_ptr<const Scene<N, T>> scene =
                simple_scene(BACKGROUND_COLOR, LIGHTING_INTENSITY, min_screen_size, max_screen_size, std::move(shape));

        static_assert(type == PainterTestOutputType::File || type == PainterTestOutputType::Window);

        if constexpr (type == PainterTestOutputType::File)
        {
                test_painter_file(samples_per_pixel, thread_count, std::move(scene));
        }

        if constexpr (type == PainterTestOutputType::Window)
        {
                test_painter_window(samples_per_pixel, thread_count, std::move(scene));
        }
}

template <size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, int point_count, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::unique_ptr<const Shape<N, T>> mesh = sphere_mesh<N, T>(point_count, &progress);

        test_painter<type>(std::move(mesh), min_screen_size, max_screen_size, samples_per_pixel, thread_count);
}

template <size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, const std::string& file_name, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::unique_ptr<const Shape<N, T>> mesh = file_mesh<N, T>(file_name, &progress);

        test_painter<type>(std::move(mesh), min_screen_size, max_screen_size, samples_per_pixel, thread_count);
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
}
