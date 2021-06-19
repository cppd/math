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

#include "process.h"

#include "../dialogs/painter_image.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/image/alpha.h>
#include <src/image/depth.h>
#include <src/image/file.h>
#include <src/image/flip.h>
#include <src/image/normalize.h>
#include <src/model/volume_utility.h>
#include <src/process/dimension.h>
#include <src/process/load.h>
#include <src/progress/progress.h>

namespace ns::gui::painter_window
{
namespace
{
template <std::size_t N, typename T>
std::array<T, N> to_array(const std::vector<T>& vector)
{
        ASSERT(vector.size() == N);
        std::array<T, N> array;
        for (std::size_t i = 0; i < N; ++i)
        {
                array[i] = vector[i];
        }
        return array;
}

template <std::size_t N_IMAGE>
void save_image(const std::filesystem::path& path, image::Image<N_IMAGE>&& image, ProgressRatio* progress)
{
        static_assert(N_IMAGE >= 2);

        progress->set(0);

        image::flip_vertically(&image);

        image::ImageView<N_IMAGE> image_view(image);
        if constexpr (N_IMAGE == 2)
        {
                image::save(path, image_view);
        }
        else
        {
                volume::save_to_images(path, image_view, progress);
        }
}

template <std::size_t N_IMAGE>
void add_volume(image::Image<N_IMAGE>&& image, ProgressRatio* progress)
{
        static_assert(N_IMAGE >= 3);

        progress->set(0);

        if (image::format_component_count(image.color_format) == 3)
        {
                constexpr float ALPHA = 1;
                image::add_alpha(image.color_format, image.pixels, ALPHA);
        }

        process::load_volume<N_IMAGE>("Painter Image", std::move(image));
}

struct Parameters final
{
        bool background;
        bool to_8_bit;

        std::string to_string() const
        {
                std::string s;
                s += background ? "with_background" : "transparent";
                if (to_8_bit)
                {
                        s += "_8-bits";
                }
                return s;
        }
};

std::vector<Parameters> create_parameters(const dialog::PainterImageParameters& dialog_parameters)
{
        if (dialog_parameters.all)
        {
                std::vector<Parameters> parameters;
                for (bool background : {false, true})
                {
                        for (bool to_8_bit : {false, true})
                        {
                                parameters.push_back({.background = background, .to_8_bit = to_8_bit});
                        }
                }
                return parameters;
        }

        return {{.background = dialog_parameters.with_background, .to_8_bit = dialog_parameters.convert_to_8_bit}};
}

template <std::size_t N_IMAGE>
image::Image<N_IMAGE> create_image(
        const Parameters& parameters,
        const std::array<int, N_IMAGE>& size,
        const image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        const image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba)
{
        static_assert(N_IMAGE >= 2);

        image::Image<N_IMAGE> image;

        image.size = size;

        if (parameters.background)
        {
                image.color_format = color_format_rgb;
                image.pixels = std::move(pixels_rgb);
        }
        else
        {
                image.color_format = color_format_rgba;
                image.pixels = std::move(pixels_rgba);
        }

        image::normalize(image.color_format, &image.pixels);

        if (parameters.to_8_bit)
        {
                image = image::convert_to_8_bit(image);
        }

        return image;
}

template <std::size_t N_IMAGE>
void save_image(
        const std::filesystem::path& path,
        const std::vector<Parameters>& parameters,
        const std::array<int, N_IMAGE>& size,
        const image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        const image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba,
        ProgressRatio* progress)
{
        if (parameters.size() == 1)
        {
                save_image(
                        path,
                        create_image(
                                parameters.front(), size, color_format_rgb, std::move(pixels_rgb), color_format_rgba,
                                std::move(pixels_rgba)),
                        progress);
                return;
        }

        if (parameters.empty())
        {
                error("No parameters for image saving");
        }

        const auto image_directory = [&path](const Parameters& p)
        {
                std::filesystem::path directory = path / path_from_utf8(p.to_string());
                if (std::filesystem::exists(directory))
                {
                        error("Path exists " + generic_utf8_filename(directory));
                }
                std::filesystem::create_directory(directory);
                return directory;
        };

        for (std::size_t i = 0; i < parameters.size() - 1; ++i)
        {
                save_image(
                        image_directory(parameters[i]),
                        create_image(
                                parameters[i], size, color_format_rgb, std::vector(pixels_rgb), color_format_rgba,
                                std::vector(pixels_rgba)),
                        progress);
        }
        save_image(
                image_directory(parameters.back()),
                create_image(
                        parameters.back(), size, color_format_rgb, std::move(pixels_rgb), color_format_rgba,
                        std::move(pixels_rgba)),
                progress);
}
}

std::function<void(ProgressRatioList*)> save_image(
        const int width,
        const int height,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba)
{
        std::optional<dialog::PainterImageParameters> dialog_parameters =
                dialog::PainterImageDialog::show("Save Image", dialog::PainterImagePathType::File, false /*use_all*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        if (!dialog_parameters->path_string)
        {
                error("No file name");
        }
        ASSERT(!dialog_parameters->all);

        return [parameters = create_parameters(*dialog_parameters),
                path_string = std::move(*dialog_parameters->path_string), width, height, color_format_rgb,
                color_format_rgba, pixels_rgb = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgb)),
                pixels_rgba = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgba))](
                       ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");

                ASSERT(parameters.size() == 1);
                save_image(
                        path_from_utf8(path_string),
                        create_image(
                                parameters.front(), std::array<int, 2>{width, height}, color_format_rgb,
                                std::move(*pixels_rgb), color_format_rgba, std::move(*pixels_rgba)),
                        &progress);
        };
}

std::function<void(ProgressRatioList*)> save_image(
        const std::vector<int>& size,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba)
{
        if (size.size() < 3)
        {
                error("Error image dimension " + to_string(size.size()) + " for saving image");
        }

        std::optional<dialog::PainterImageParameters> dialog_parameters = dialog::PainterImageDialog::show(
                "Save All Images", dialog::PainterImagePathType::Directory, true /*use_all*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        if (!dialog_parameters->path_string)
        {
                error("No directory name");
        }

        return [parameters = create_parameters(*dialog_parameters),
                path_string = std::move(*dialog_parameters->path_string), size, color_format_rgb, color_format_rgba,
                pixels_rgb = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgb)),
                pixels_rgba = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgba))](
                       ProgressRatioList* progress_list)
        {
                const int N = size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<std::size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        ProgressRatio progress(progress_list, "Saving");

                                        ASSERT(!parameters.empty());
                                        save_image(
                                                path_from_utf8(path_string), parameters, to_array<N_IMAGE, int>(size),
                                                color_format_rgb, std::move(*pixels_rgb), color_format_rgba,
                                                std::move(*pixels_rgba), &progress);
                                }
                        });
        };
}

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& size,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba)
{
        if (size.size() < 3)
        {
                error("Error image dimension " + to_string(size.size()) + " for adding volume");
        }

        std::optional<dialog::PainterImageParameters> dialog_parameters =
                dialog::PainterImageDialog::show("Add Volume", dialog::PainterImagePathType::None, false /*use_all*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        ASSERT(!dialog_parameters->path_string.has_value());
        ASSERT(!dialog_parameters->all);

        return [parameters = create_parameters(*dialog_parameters), size, color_format_rgb, color_format_rgba,
                pixels_rgb = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgb)),
                pixels_rgba = std::make_shared<std::vector<std::byte>>(std::move(pixels_rgba))](
                       ProgressRatioList* progress_list)
        {
                const int N = size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<std::size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        ProgressRatio progress(progress_list, "Adding volume");

                                        ASSERT(parameters.size() == 1);
                                        add_volume<N_IMAGE>(
                                                create_image(
                                                        parameters.front(), to_array<N_IMAGE, int>(size),
                                                        color_format_rgb, std::move(*pixels_rgb), color_format_rgba,
                                                        std::move(*pixels_rgba)),
                                                &progress);
                                }
                        });
        };
}
}
