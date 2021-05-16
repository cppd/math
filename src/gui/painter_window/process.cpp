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
#include <src/image/grayscale.h>
#include <src/model/volume_utility.h>
#include <src/process/dimension.h>
#include <src/process/load.h>
#include <src/progress/progress.h>

namespace ns::gui::painter_window
{
namespace
{
constexpr std::string_view IMAGE_FILE_FORMAT = "png";

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

struct Parameters final
{
        bool background;
        bool grayscale;
        bool to_8_bit;

        std::string to_string() const
        {
                std::string s;
                s += background ? "with_background" : "transparent";
                if (grayscale)
                {
                        s += "_grayscale";
                }
                if (to_8_bit)
                {
                        s += "_8-bits";
                }
                return s;
        }
};

template <std::size_t N_IMAGE>
image::Image<N_IMAGE> create_image(
        const bool delete_alpha,
        const std::array<int, N_IMAGE>& size,
        const Color& background,
        const image::ColorFormat color_format,
        const Parameters& parameters,
        std::vector<std::byte>&& pixels)
{
        static_assert(N_IMAGE >= 2);

        image::Image<N_IMAGE> image;
        image.size = size;
        image.color_format = color_format;
        image.pixels = std::move(pixels);

        ASSERT(image::format_component_count(color_format) == 4);

        if (parameters.background && !parameters.grayscale)
        {
                image::blend_alpha(&image.color_format, image.pixels, background);
                if (delete_alpha)
                {
                        image = image::delete_alpha(image);
                }
                else
                {
                        constexpr float ALPHA = 1;
                        image::set_alpha(image.color_format, image.pixels, ALPHA);
                }
        }
        else if (parameters.background && parameters.grayscale)
        {
                image::blend_alpha(&image.color_format, image.pixels, background);
                image::make_grayscale(image.color_format, image.pixels);
                image = image::convert_to_r_component_format(image);
        }
        else if (!parameters.background && parameters.grayscale)
        {
                image::make_grayscale(image.color_format, image.pixels);
                image::blend_alpha(&image.color_format, image.pixels, Color(0));
                image = image::convert_to_r_component_format(image);
        }

        ASSERT(!(parameters.grayscale && parameters.to_8_bit));
        if (parameters.to_8_bit)
        {
                image = image::convert_to_8_bit(image);
        }

        return image;
}

template <std::size_t N_IMAGE>
void save_image(
        const std::array<int, N_IMAGE>& size,
        const Color& background,
        const image::ColorFormat color_format,
        const std::filesystem::path& path,
        const Parameters& parameters,
        std::vector<std::byte>&& pixels,
        ProgressRatio* progress)
{
        static_assert(N_IMAGE >= 2);

        constexpr bool DELETE_ALPHA = true;

        progress->set(0);

        image::Image<N_IMAGE> image =
                create_image(DELETE_ALPHA, size, background, color_format, parameters, std::move(pixels));

        image::flip_vertically(&image);

        image::ImageView<N_IMAGE> image_view(image);
        if constexpr (N_IMAGE == 2)
        {
                image::save(path, image_view);
        }
        else
        {
                volume::save_to_images(path, IMAGE_FILE_FORMAT, image_view, progress);
        }
}

template <std::size_t N_IMAGE>
void add_volume(
        const std::array<int, N_IMAGE>& size,
        const Color& background,
        const image::ColorFormat color_format,
        const Parameters& parameters,
        std::vector<std::byte>&& pixels,
        ProgressRatio* progress)
{
        static_assert(N_IMAGE >= 3);

        constexpr bool DELETE_ALPHA = false;

        progress->set(0);

        image::Image<N_IMAGE> image =
                create_image(DELETE_ALPHA, size, background, color_format, parameters, std::move(pixels));

        process::load_volume<N_IMAGE>("Painter Image", std::move(image));
}

template <std::size_t N_IMAGE>
void save_image(
        const std::array<int, N_IMAGE>& size,
        const Color& background,
        const image::ColorFormat color_format,
        const std::filesystem::path& path,
        const std::vector<Parameters>& parameters,
        std::vector<std::byte>&& pixels,
        ProgressRatio* progress)
{
        if (parameters.size() == 1)
        {
                save_image(size, background, color_format, path, parameters.front(), std::move(pixels), progress);
                return;
        }

        const auto save = [&size, &background, &color_format, &path,
                           &progress](const Parameters& image_parameters, std::vector<std::byte>&& image_pixels)
        {
                const std::filesystem::path image_path = path / path_from_utf8(image_parameters.to_string());
                if (std::filesystem::exists(image_path))
                {
                        error("Path exists " + generic_utf8_filename(image_path));
                }
                std::filesystem::create_directory(image_path);

                save_image(
                        size, background, color_format, image_path, image_parameters, std::move(image_pixels),
                        progress);
        };

        if (parameters.empty())
        {
                error("No parameters for image saving");
        }
        for (std::size_t i = 0; i < parameters.size() - 1; ++i)
        {
                save(parameters[i], std::vector(pixels));
        }
        save(parameters.back(), std::move(pixels));
}

std::vector<Parameters> create_parameters(const dialog::PainterImageParameters& dialog_parameters)
{
        if (dialog_parameters.all)
        {
                return {{.background = false, .grayscale = false, .to_8_bit = false},
                        {.background = false, .grayscale = false, .to_8_bit = true},
                        {.background = false, .grayscale = true, .to_8_bit = false},
                        {.background = true, .grayscale = false, .to_8_bit = false},
                        {.background = true, .grayscale = false, .to_8_bit = true},
                        {.background = true, .grayscale = true, .to_8_bit = false}};
        }

        return {
                {.background = dialog_parameters.with_background,
                 .grayscale = dialog_parameters.grayscale,
                 .to_8_bit = dialog_parameters.convert_to_8_bit}};
}
}

std::function<void(ProgressRatioList*)> save_image(
        const int width,
        const int height,
        const Color& background,
        const image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        std::optional<dialog::PainterImageParameters> dialog_parameters = dialog::PainterImageDialog::show(
                "Save Image", dialog::PainterImagePathType::File, false /*use_all*/, false /*use_grayscale*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        if (!dialog_parameters->path_string)
        {
                error("No file name");
        }
        ASSERT(!dialog_parameters->all);
        ASSERT(!dialog_parameters->grayscale);
        dialog_parameters->grayscale = false;

        return [width, height, background, color_format, parameters = create_parameters(*dialog_parameters),
                path_string = std::move(*dialog_parameters->path_string),
                pixels = std::make_shared<std::vector<std::byte>>(std::move(pixels))](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");

                ASSERT(parameters.size() == 1);
                save_image(
                        std::array<int, 2>{width, height}, background, color_format, path_from_utf8(path_string),
                        parameters.front(), std::move(*pixels), &progress);
        };
}

std::function<void(ProgressRatioList*)> save_image(
        const std::vector<int>& size,
        const Color& background,
        const image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        if (size.size() < 3)
        {
                error("Error image dimension " + to_string(size.size()) + " for saving image");
        }

        std::optional<dialog::PainterImageParameters> dialog_parameters = dialog::PainterImageDialog::show(
                "Save All Images", dialog::PainterImagePathType::Directory, true /*use_all*/, true /*use_grayscale*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        if (!dialog_parameters->path_string)
        {
                error("No directory name");
        }

        return [size, background, color_format, parameters = create_parameters(*dialog_parameters),
                path_string = std::move(*dialog_parameters->path_string),
                pixels = std::make_shared<std::vector<std::byte>>(std::move(pixels))](ProgressRatioList* progress_list)
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
                                                to_array<N_IMAGE, int>(size), background, color_format,
                                                path_from_utf8(path_string), parameters, std::move(*pixels), &progress);
                                }
                        });
        };
}

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& size,
        const Color& background,
        const image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        if (size.size() < 3)
        {
                error("Error image dimension " + to_string(size.size()) + " for adding volume");
        }

        std::optional<dialog::PainterImageParameters> dialog_parameters = dialog::PainterImageDialog::show(
                "Add Volume", dialog::PainterImagePathType::None, false /*use_all*/, true /*use_grayscale*/);
        if (!dialog_parameters)
        {
                return nullptr;
        }
        ASSERT(!dialog_parameters->path_string.has_value());
        ASSERT(!dialog_parameters->all);

        return [size, background, color_format, parameters = create_parameters(*dialog_parameters),
                pixels = std::make_shared<std::vector<std::byte>>(std::move(pixels))](ProgressRatioList* progress_list)
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
                                                to_array<N_IMAGE, int>(size), background, color_format,
                                                parameters.front(), std::move(*pixels), &progress);
                                }
                        });
        };
}
}
