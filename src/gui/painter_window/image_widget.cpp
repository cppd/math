/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "image_widget.h"

#include <src/com/error.h>

#include <QImage>
#include <QMenu>
#include <QPixmap>
#include <QSize>
#include <QWidget>
#include <Qt>

#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

namespace ns::gui::painter_window
{
namespace
{
constexpr bool SHOW_THREADS = true;
constexpr long long PIXEL_SIZE_BYTES = 4;
}

ImageWidget::ImageWidget(const int width, const int height, QMenu* const menu)
        : QWidget(nullptr),
          image_2d_pixel_count_(1ull * width * height),
          image_2d_byte_count_(PIXEL_SIZE_BYTES * image_2d_pixel_count_),
          image_2d_(width, height, QImage::Format_RGBX8888)
{
        ASSERT(static_cast<std::size_t>(image_2d_.sizeInBytes()) == image_2d_byte_count_);

        ui_.setupUi(this);

        layout()->setContentsMargins(0, 0, 0, 0);

        ui_.label_image->setText("");

        ui_.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui_.scrollAreaWidgetContents->layout()->setSpacing(0);

        ui_.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui_.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        show_threads_action_ = menu->addAction("Show threads");
        show_threads_action_->setCheckable(true);
        show_threads_action_->setChecked(SHOW_THREADS);
}

QSize ImageWidget::size_difference() const
{
        ui_.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui_.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        const QSize difference = image_2d_.size() / ui_.label_image->devicePixelRatioF() - ui_.scroll_area->size();
        ui_.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui_.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        return difference;
}

void ImageWidget::update(const std::span<const std::byte> pixels_r8g8b8a8, const std::vector<long long>& busy_indices)
{
        unsigned char* const image_bits = image_2d_.bits();

        ASSERT(pixels_r8g8b8a8.size_bytes() == image_2d_byte_count_);
        std::memcpy(image_bits, pixels_r8g8b8a8.data(), image_2d_byte_count_);

        if (show_threads_action_->isChecked())
        {
                for (long long index : busy_indices)
                {
                        if (index >= 0)
                        {
                                ASSERT(index < image_2d_pixel_count_);
                                index *= PIXEL_SIZE_BYTES;
                                static_assert(PIXEL_SIZE_BYTES >= 3);
                                image_bits[index++] ^= 0xff;
                                image_bits[index++] ^= 0xff;
                                image_bits[index] ^= 0xff;
                        }
                }
        }

        if (!(image_2d_.devicePixelRatioF() == ui_.label_image->devicePixelRatioF()))
        {
                image_2d_.setDevicePixelRatio(ui_.label_image->devicePixelRatioF());
        }
        ui_.label_image->setPixmap(QPixmap::fromImage(image_2d_));
        ui_.label_image->update();
}
}
