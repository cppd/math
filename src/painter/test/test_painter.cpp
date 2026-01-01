/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/color/color.h>
#include <src/color/rgb8.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/com/thread.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/gui/painter_window/painter_window.h>
#include <src/image/conversion.h>
#include <src/image/flip.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/model/volume_utility.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/painter/painter.h>
#include <src/painter/scenes/simple.h>
#include <src/painter/scenes/storage.h>
#include <src/painter/shapes/mesh.h>
#include <src/progress/progress.h>
#include <src/settings/directory.h>

#include <QApplication>

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::painter
{
namespace
{
constexpr color::RGB8 BACKGROUND_LIGHT(50, 100, 150);
constexpr float LIGHTING_INTENSITY = 1;
constexpr float FRONT_LIGHT_PROPORTION = 0.2;

constexpr std::string_view DIRECTORY_NAME = "painter_test";
constexpr image::ColorFormat PIXEL_COLOR_FORMAT = image::ColorFormat::R8G8B8_SRGB;

constexpr Integrator INTEGRATOR = Integrator::PT;

constexpr bool WRITE_LOG = false;

template <std::size_t N>
void save_image(const std::filesystem::path& path, image::Image<N>&& image)
{
        image::flip_vertically(&image);

        {
                std::vector<std::byte> pixels;
                image::format_conversion(image.color_format, image.pixels, PIXEL_COLOR_FORMAT, &pixels);
                image.color_format = PIXEL_COLOR_FORMAT;
                image.pixels = std::move(pixels);
        }

        progress::Ratio progress(nullptr);
        model::volume::save_to_images(path, image::ImageView(image), &progress);
}

template <std::size_t N>
class Image final : public Notifier<N>
{
        const std::filesystem::path path_;

        std::unique_ptr<Images<N>> images_ = std::make_unique<Images<N>>();
        std::atomic_bool images_ready_ = false;

        void thread_busy(unsigned, const std::array<int, N>&) override
        {
        }

        void thread_free(unsigned) override
        {
        }

        void pixel_set(const std::array<int, N>&, const numerical::Vector<3, float>&) override
        {
        }

        Images<N>* images(long long) override
        {
                return images_.get();
        }

        void pass_done(long long) override
        {
                images_ready_ = true;
        }

        void error_message(const std::string& msg) override
        {
                LOG("Painter error message\n" + msg);
        }

public:
        explicit Image(const std::string_view directory_name)
                : path_(settings::test_path(directory_name))
        {
                if (!std::filesystem::create_directory(path_))
                {
                        error("Error creating directory " + generic_utf8_filename(path_));
                }
        }

        void write_to_files()
        {
                if (!images_ready_)
                {
                        error("No painter image to write to files");
                }
                const ImagesWriting lock(images_.get());
                save_image(path_, std::move(lock.image_with_background()));
        }
};

template <std::size_t N>
float random_radius()
{
        static constexpr std::array EXPONENTS = std::to_array<std::array<int, 2>>({
                {-7, 10},
                {-4,  6},
                {-3,  5},
                {-2,  3}
        });

        static_assert(N >= 3 && N < 3 + EXPONENTS.size());
        static constexpr std::array<int, 2> MIN_MAX = EXPONENTS[N - 3];

        PCG engine;
        const float exponent = std::uniform_real_distribution<float>(MIN_MAX[0], MIN_MAX[1])(engine);
        return std::pow(10.0f, exponent);
}

template <std::size_t N>
std::unique_ptr<const model::mesh::Mesh<N>> sphere_mesh(const int facet_count)
{
        LOG("creating sphere in " + space_name(N) + "...");

        std::vector<numerical::Vector<N, float>> vertices;
        std::vector<std::array<int, N>> facets;
        geometry::shapes::create_sphere(facet_count, &vertices, &facets);

        const float radius = random_radius<N>();
        const numerical::Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));
        LOG("facet count " + to_string(facets.size()));

        for (numerical::Vector<N, float>& v : vertices)
        {
                v = radius * v + center;
        }

        LOG("creating mesh...");
        return model::mesh::create_mesh_for_facets(vertices, facets, WRITE_LOG);
}

template <std::size_t N>
std::unique_ptr<const model::mesh::Mesh<N>> file_mesh(const std::string& file_name, progress::Ratio* const progress)
{
        LOG("Loading geometry from file...");
        return model::mesh::load<N>(path_from_utf8(file_name), progress);
}

template <std::size_t N, typename T, typename Color>
void test_painter_file(const int samples_per_pixel, const int thread_count, scenes::StorageScene<N, T, Color>&& scene)
{
        constexpr int MAX_PASS_COUNT = 1;
        constexpr bool FLAT_SHADING = false;

        Image<N - 1> image(DIRECTORY_NAME);

        LOG("Painting...");
        const Clock::time_point start_time = Clock::now();
        {
                std::unique_ptr<Painter> painter = create_painter(
                        INTEGRATOR, &image, samples_per_pixel, MAX_PASS_COUNT, scene.scene.get(), thread_count,
                        FLAT_SHADING);
                painter->wait();
        }
        LOG("Painted, " + to_string_fixed(duration_from(start_time), 5) + " s");

        LOG("Writing screen images to files...");
        image.write_to_files();

        LOG("Done");
}

template <std::size_t N, typename T, typename Color>
void test_painter_window(const int samples_per_pixel, const int thread_count, scenes::StorageScene<N, T, Color>&& scene)
{
        constexpr bool FLAT_SHADING = false;

        LOG("Window painting...");

        if (!QApplication::instance())
        {
                error("No application object for path tracing tests");
        }

        std::string name = "Path Tracing In " + to_upper_first_letters(space_name(N));

        gui::painter_window::create_painter_window(
                std::move(name), INTEGRATOR, thread_count, samples_per_pixel, FLAT_SHADING, std::move(scene));
}

enum class OutputType
{
        FILE,
        WINDOW
};

template <OutputType OUTPUT_TYPE, std::size_t N, typename T, typename Color>
void test_painter(
        std::unique_ptr<const model::mesh::Mesh<N>>&& mesh,
        progress::Ratio* const progress,
        const int max_screen_size,
        const int samples_per_pixel,
        const int thread_count)
{
        std::unique_ptr<const Shape<N, T, Color>> painter_mesh;
        {
                const model::mesh::MeshObject<N> mesh_object(
                        std::move(mesh), numerical::IDENTITY_MATRIX<N + 1, double>, "");
                std::vector<const model::mesh::MeshObject<N>*> mesh_objects;
                mesh_objects.push_back(&mesh_object);

                static constexpr std::optional<numerical::Vector<N + 1, T>> CLIP_PLANE_EQUATION;

                painter_mesh = shapes::create_mesh<N, T, Color>(mesh_objects, CLIP_PLANE_EQUATION, WRITE_LOG, progress);
        }

        scenes::StorageScene<N, T, Color> scene = scenes::create_simple_scene(
                std::move(painter_mesh), Color::illuminant(LIGHTING_INTENSITY, LIGHTING_INTENSITY, LIGHTING_INTENSITY),
                Color::illuminant(BACKGROUND_LIGHT), std::nullopt, FRONT_LIGHT_PROPORTION, max_screen_size, progress);

        static_assert(OUTPUT_TYPE == OutputType::FILE || OUTPUT_TYPE == OutputType::WINDOW);

        if constexpr (OUTPUT_TYPE == OutputType::FILE)
        {
                test_painter_file(samples_per_pixel, thread_count, std::move(scene));
        }

        if constexpr (OUTPUT_TYPE == OutputType::WINDOW)
        {
                test_painter_window(samples_per_pixel, thread_count, std::move(scene));
        }
}

template <std::size_t N, typename T, typename Color, OutputType OUTPUT_TYPE>
void test_painter(const int samples_per_pixel, const int facet_count, const int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        progress::Ratio progress(nullptr);

        std::unique_ptr<const model::mesh::Mesh<N>> mesh = sphere_mesh<N>(facet_count);

        test_painter<OUTPUT_TYPE, N, T, Color>(
                std::move(mesh), &progress, max_screen_size, samples_per_pixel, thread_count);
}

template <std::size_t N, typename T, typename Color, OutputType OUTPUT_TYPE>
void test_painter(const int samples_per_pixel, const std::string& file_name, const int max_screen_size)
{
        const int thread_count = hardware_concurrency();
        progress::Ratio progress(nullptr);

        std::unique_ptr<const model::mesh::Mesh<N>> mesh = file_mesh<N>(file_name, &progress);

        test_painter<OUTPUT_TYPE, N, T, Color>(
                std::move(mesh), &progress, max_screen_size, samples_per_pixel, thread_count);
}
}

void test_painter_file()
{
        constexpr std::size_t N = 4;
        constexpr int SAMPLES_PER_PIXEL = 25;
        test_painter<N, double, color::Spectrum, OutputType::FILE>(SAMPLES_PER_PIXEL, 1000, 100);
}

void test_painter_file(const std::string& file_name)
{
        constexpr std::size_t N = 4;
        constexpr int SAMPLES_PER_PIXEL = 25;
        test_painter<N, double, color::Spectrum, OutputType::FILE>(SAMPLES_PER_PIXEL, file_name, 100);
}

void test_painter_window()
{
        constexpr std::size_t N = 4;
        constexpr int SAMPLES_PER_PIXEL = 25;
        test_painter<N, double, color::Spectrum, OutputType::WINDOW>(SAMPLES_PER_PIXEL, 1000, 500);
}

void test_painter_window(const std::string& file_name)
{
        constexpr std::size_t N = 4;
        constexpr int SAMPLES_PER_PIXEL = 25;
        test_painter<N, double, color::Spectrum, OutputType::WINDOW>(SAMPLES_PER_PIXEL, file_name, 500);
}
}
