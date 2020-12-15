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

#include "process.h"

#include "../dialogs/painter_image.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/alpha.h>
#include <src/image/file.h>
#include <src/image/flip.h>
#include <src/model/volume_utility.h>
#include <src/process/dimension.h>
#include <src/process/load.h>
#include <src/progress/progress.h>
#include <src/utility/file/path.h>

#include <cstring>

namespace gui::painter_window
{
namespace
{
constexpr const char* IMAGE_FILE_FORMAT = "png";

template <size_t N, typename T>
std::array<T, N> to_array(const std::vector<T>& vector)
{
        ASSERT(vector.size() == N);
        std::array<T, N> array;
        for (size_t i = 0; i < N; ++i)
        {
                array[i] = vector[i];
        }
        return array;
}
}

std::function<void(ProgressRatioList*)> save_to_file(
        const std::vector<int>& screen_size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        std::optional<dialog::PainterImageParameters> parameters =
                dialog::PainterImageDialog::show("Save", dialog::PainterImagePathType::File);
        if (!parameters)
        {
                return nullptr;
        }

        return [=, pixels_ptr = std::make_shared<std::vector<std::byte>>(std::move(pixels)),
                parameters = std::move(*parameters)](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");
                progress.set(0);

                image::Image<2> image;
                image.size = {screen_size[0], screen_size[1]};
                image.color_format = color_format;
                image.pixels = std::move(*pixels_ptr);

                ASSERT(image::format_component_count(color_format) == 4);
                if (parameters.with_background)
                {
                        image::blend_alpha(color_format, image.pixels, background);
                        image = image::delete_alpha(image);
                }

                image::save(path_from_utf8(*parameters.path_string), image::ImageView<2>(image));
        };
}

std::function<void(ProgressRatioList*)> save_all_to_files(
        const std::vector<int>& screen_size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        if (screen_size.size() < 3)
        {
                return nullptr;
        }

        std::optional<dialog::PainterImageParameters> parameters =
                dialog::PainterImageDialog::show("Save all", dialog::PainterImagePathType::Directory);
        if (!parameters)
        {
                return nullptr;
        }

        return [=, pixels_ptr = std::make_shared<std::vector<std::byte>>(std::move(pixels)),
                parameters = std::move(*parameters)](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Saving");
                progress.set(0);

                const int N = screen_size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        image::Image<N_IMAGE> image;
                                        image.size = to_array<N_IMAGE, int>(screen_size);
                                        image.color_format = color_format;
                                        image.pixels = std::move(*pixels_ptr);

                                        ASSERT(image::format_component_count(color_format) == 4);
                                        if (parameters.with_background)
                                        {
                                                image::blend_alpha(color_format, image.pixels, background);
                                                image = image::delete_alpha(image);
                                        }

                                        volume::save_to_images(
                                                path_from_utf8(*parameters.path_string), IMAGE_FILE_FORMAT,
                                                image::ImageView<N_IMAGE>(image), &progress);
                                }
                        });
        };
}

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& screen_size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels)
{
        if (screen_size.size() < 3)
        {
                return nullptr;
        }

        std::optional<dialog::PainterImageParameters> parameters =
                dialog::PainterImageDialog::show("Add volume", dialog::PainterImagePathType::None);
        if (!parameters)
        {
                return nullptr;
        }

        return [=, pixels_ptr = std::make_shared<std::vector<std::byte>>(std::move(pixels)),
                parameters = std::move(*parameters)](ProgressRatioList* progress_list)
        {
                ProgressRatio progress(progress_list, "Adding volume");
                progress.set(0);

                const int N = screen_size.size() + 1;
                process::apply_for_dimension(
                        N,
                        [&]<size_t N>(const process::Dimension<N>&)
                        {
                                constexpr int N_IMAGE = N - 1;
                                if constexpr (N_IMAGE >= 3)
                                {
                                        image::Image<N_IMAGE> image;
                                        image.size = to_array<N_IMAGE, int>(screen_size);
                                        image.color_format = color_format;
                                        image.pixels = std::move(*pixels_ptr);

                                        image::flip_vertically(&image);

                                        ASSERT(image::format_component_count(color_format) == 4);
                                        if (parameters.with_background)
                                        {
                                                image::blend_alpha(color_format, image.pixels, background);
                                                constexpr float ALPHA = 1;
                                                image::set_alpha(color_format, image.pixels, ALPHA);
                                        }

                                        process::load_volume<N_IMAGE>("Painter Image", std::move(image));
                                }
                        });
        };
}

}
