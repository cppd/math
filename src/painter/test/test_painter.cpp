/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/file/path.h>
#include <src/com/global_index.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/string/str.h>
#include <src/com/time.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/gui/com/support.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume_utility.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <random>

namespace ns::painter
{
namespace
{
constexpr Color BACKGROUND_COLOR = Color(Srgb8(50, 100, 150));
constexpr Color DEFAULT_COLOR = Color(Srgb8(150, 170, 150));
constexpr Color::DataType METALNESS = 0;
constexpr Color::DataType LIGHTING_INTENSITY = 1;

template <std::size_t N>
class Images final : public Notifier<N>
{
        static constexpr const char* DIRECTORY_NAME = "painter_test";
        static constexpr const char* IMAGE_FILE_FORMAT = "png";

        const GlobalIndex<N, long long> m_global_index;
        const std::array<int, N> m_screen_size;
        const Color m_background_color;

        std::vector<std::byte> m_pixels;
        std::filesystem::path m_directory;

        void pixel_busy(unsigned, const std::array<int_least16_t, N>&) override
        {
        }

        void pixel_set(unsigned, const std::array<int_least16_t, N>& pixel, const Color& color, float coverage) override
        {
                std::array<int_least16_t, N> p = pixel;
                p[1] = m_screen_size[1] - 1 - pixel[1];

                Color c = interpolation(m_background_color, color, coverage);
                unsigned char r = color::linear_float_to_srgb_uint8(c.red());
                unsigned char g = color::linear_float_to_srgb_uint8(c.green());
                unsigned char b = color::linear_float_to_srgb_uint8(c.blue());

                std::size_t offset = 3 * m_global_index.compute(pixel);
                ASSERT(offset + 2 < m_pixels.size());
                std::byte* ptr = m_pixels.data() + offset;
                std::memcpy(ptr++, &r, 1);
                std::memcpy(ptr++, &g, 1);
                std::memcpy(ptr++, &b, 1);
        }

        void error_message(const std::string& msg) override
        {
                LOG("Painter error message\n" + msg);
        }

public:
        Images(const std::array<int, N>& screen_size, const Color& background_color)
                : m_global_index(screen_size), m_screen_size(screen_size), m_background_color(background_color)
        {
                if (!std::all_of(
                            screen_size.cbegin(), screen_size.cend(),
                            [](int v)
                            {
                                    return v > 0;
                            }))
                {
                        error("Error screen size " + to_string(screen_size));
                }

                m_pixels.resize(3 * m_global_index.count(), std::byte(0));

                m_directory = std::filesystem::temp_directory_path() / DIRECTORY_NAME;
                if (!std::filesystem::create_directory(m_directory))
                {
                        error("Error creating directory " + generic_utf8_filename(m_directory));
                }
        }

        void write_to_files() const
        {
                ProgressRatio progress(nullptr);
                volume::save_to_images(
                        m_directory, IMAGE_FILE_FORMAT,
                        image::ImageView<N>(m_screen_size, image::ColorFormat::R8G8B8_SRGB, m_pixels), &progress);
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

template <std::size_t N>
float random_radius()
{
        static constexpr std::array EXPONENTS =
                std::to_array<std::array<int, 2>>({{-7, 10}, {-4, 6}, {-3, 5}, {-2, 3}});

        static_assert(N >= 3 && N < 3 + EXPONENTS.size());
        static constexpr std::array<int, 2> MIN_MAX = EXPONENTS[N - 3];

        std::mt19937 random_engine = create_engine<std::mt19937>();
        float exponent = std::uniform_real_distribution<float>(MIN_MAX[0], MIN_MAX[1])(random_engine);
        return std::pow(10.0f, exponent);
}

template <std::size_t N>
std::unique_ptr<const mesh::Mesh<N>> sphere_mesh(int facet_count)
{
        LOG("creating sphere in " + space_name(N) + "...");
        std::vector<Vector<N, float>> vertices;
        std::vector<std::array<int, N>> facets;
        geometry::create_sphere(facet_count, &vertices, &facets);

        const float radius = random_radius<N>();
        const Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));
        LOG("facet count " + to_string(facets.size()));

        for (Vector<N, float>& v : vertices)
        {
                v = radius * v + center;
        }

        LOG("creating mesh...");
        return mesh::create_mesh_for_facets(vertices, facets);
}

template <std::size_t N>
std::unique_ptr<const mesh::Mesh<N>> file_mesh(const std::string& file_name, ProgressRatio* progress)
{
        LOG("Loading geometry from file...");
        return mesh::load<N>(file_name, progress);
}

template <std::size_t N, typename T>
void test_painter_file(int samples_per_pixel, int thread_count, std::unique_ptr<const Scene<N, T>>&& scene)
{
        constexpr int max_pass_count = 1;
        constexpr bool smooth_normal = true;

        Images images(scene->projector().screen_size(), scene->background_color());

        LOG("Painting...");
        TimePoint start_time = time();
        {
                std::unique_ptr<Painter<N, T>> painter = create_painter<N, T>(
                        &images, samples_per_pixel, max_pass_count, std::move(scene), thread_count, smooth_normal);
                painter->wait();
        }
        LOG("Painted, " + to_string_fixed(duration_from(start_time), 5) + " s");

        LOG("Writing screen images to files...");
        images.write_to_files();

        LOG("Done");
}

template <std::size_t N, typename T>
void test_painter_window(int samples_per_pixel, int thread_count, std::unique_ptr<const Scene<N, T>>&& scene)
{
        constexpr bool smooth_normal = true;

        LOG("Window painting...");

        check_application_instance();

        std::string name = "Path Tracing In " + to_upper_first_letters(space_name(N));

        gui::painter_window::create_painter_window(
                name, thread_count, samples_per_pixel, smooth_normal, std::move(scene));
}

enum class PainterTestOutputType
{
        File,
        Window
};

template <PainterTestOutputType type, std::size_t N, typename T>
void test_painter(
        std::unique_ptr<const mesh::Mesh<N>>&& mesh,
        ProgressRatio* progress,
        int min_screen_size,
        int max_screen_size,
        int samples_per_pixel,
        int thread_count)
{
        std::unique_ptr<const Shape<N, T>> painter_mesh;
        {
                mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");
                {
                        mesh::Writing writing(&mesh_object);
                        writing.set_color(DEFAULT_COLOR);
                        writing.set_metalness(METALNESS);
                }
                std::vector<const mesh::MeshObject<N>*> mesh_objects;
                mesh_objects.push_back(&mesh_object);
                painter_mesh = std::make_unique<const Mesh<N, T>>(mesh_objects, progress);
        }

        std::unique_ptr<const Scene<N, T>> scene = simple_scene(
                BACKGROUND_COLOR, LIGHTING_INTENSITY, min_screen_size, max_screen_size, std::move(painter_mesh));

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

template <std::size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, int facet_count, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::unique_ptr<const mesh::Mesh<N>> mesh = sphere_mesh<N>(facet_count);

        test_painter<type, N, T>(
                std::move(mesh), &progress, min_screen_size, max_screen_size, samples_per_pixel, thread_count);
}

template <std::size_t N, typename T, PainterTestOutputType type>
void test_painter(int samples_per_pixel, const std::string& file_name, int min_screen_size, int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        ProgressRatio progress(nullptr);

        std::unique_ptr<const mesh::Mesh<N>> mesh = file_mesh<N>(file_name, &progress);

        test_painter<type, N, T>(
                std::move(mesh), &progress, min_screen_size, max_screen_size, samples_per_pixel, thread_count);
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
