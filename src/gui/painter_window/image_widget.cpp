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

#include "image_widget.h"

#include <src/com/error.h>

#include <cstring>

namespace ns::gui::painter_window
{
namespace
{
constexpr bool SHOW_THREADS = true;
constexpr long long PIXEL_SIZE_BYTES = 4;
}

ImageWidget::ImageWidget(int width, int height, QMenu* menu)
        : QWidget(nullptr),
          m_image_2d_pixel_count(1ull * width * height),
          m_image_2d_byte_count(PIXEL_SIZE_BYTES * m_image_2d_pixel_count),
          m_image_2d(width, height, QImage::Format_RGBX8888)
{
        ASSERT(static_cast<std::size_t>(m_image_2d.sizeInBytes()) == m_image_2d_byte_count);

        ui.setupUi(this);

        layout()->setContentsMargins(0, 0, 0, 0);

        ui.label_image->setText("");
        ui.label_image->resize(m_image_2d.width(), m_image_2d.height());

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);

        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_show_threads_action = menu->addAction("Show threads");
        m_show_threads_action->setCheckable(true);
        m_show_threads_action->setChecked(SHOW_THREADS);
}

QSize ImageWidget::size_difference() const
{
        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QSize size = m_image_2d.size() - ui.scroll_area->size();
        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        return size;
}

void ImageWidget::update(const std::span<const std::byte>& pixels_r8g8b8a8, const std::vector<long long>& busy_indices)
{
        unsigned char* const image_bits = m_image_2d.bits();

        ASSERT(pixels_r8g8b8a8.size_bytes() == m_image_2d_byte_count);
        std::memcpy(image_bits, pixels_r8g8b8a8.data(), m_image_2d_byte_count);

        if (m_show_threads_action->isChecked())
        {
                for (long long index : busy_indices)
                {
                        if (index >= 0)
                        {
                                ASSERT(index < m_image_2d_pixel_count);
                                index *= PIXEL_SIZE_BYTES;
                                static_assert(PIXEL_SIZE_BYTES >= 3);
                                image_bits[index++] ^= 0xff;
                                image_bits[index++] ^= 0xff;
                                image_bits[index] ^= 0xff;
                        }
                }
        }

        ui.label_image->setPixmap(QPixmap::fromImage(m_image_2d));
        ui.label_image->update();
}
}
