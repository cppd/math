/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "opengl_memory.h"

namespace gpgpu_optical_flow_compute_opengl_implementation
{
GrayscaleMemory::GrayscaleMemory(const opengl::TextureRGBA32F& image_src, const opengl::TextureR32F& image_dst)
        : m_buffer(sizeof(Images))
{
        Images images;

        images.image_src = image_src.image_resident_handle_read_only();
        images.image_dst = image_dst.image_resident_handle_write_only();

        m_buffer.copy(images);
}

void GrayscaleMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

DownsampleMemory::DownsampleMemory(const opengl::TextureR32F& image_big, const opengl::TextureR32F& image_small)
        : m_buffer(sizeof(Images))
{
        Images images;

        images.image_big = image_big.image_resident_handle_read_only();
        images.image_small = image_small.image_resident_handle_write_only();

        m_buffer.copy(images);
}

void DownsampleMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

SobelMemory::SobelMemory(const opengl::TextureR32F& image_i, const opengl::TextureR32F& image_dx,
                         const opengl::TextureR32F& image_dy)
        : m_buffer(sizeof(Images))
{
        Images images;

        images.image_i = image_i.image_resident_handle_read_only();
        images.image_dx = image_dx.image_resident_handle_write_only();
        images.image_dy = image_dy.image_resident_handle_write_only();

        m_buffer.copy(images);
}

void SobelMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}

//

FlowDataMemory::FlowDataMemory() : m_buffer(sizeof(Data))
{
}

void FlowDataMemory::set_top_points(const opengl::StorageBuffer* top_points)
{
        m_top_points = top_points;
}

void FlowDataMemory::set_flow_guess(const opengl::StorageBuffer* flow_guess)
{
        m_flow_guess = flow_guess;
}

void FlowDataMemory::set_flow(const opengl::StorageBuffer* flow)
{
        m_flow = flow;
}

void FlowDataMemory::set_data(const Data& data)
{
        m_buffer.copy(data);
}

void FlowDataMemory::bind() const
{
        ASSERT(m_flow);

        if (m_top_points)
        {
                m_top_points->bind(POINTS_BINDING);
        }

        m_flow->bind(POINTS_FLOW_BINDING);

        if (m_flow_guess)
        {
                m_flow_guess->bind(POINTS_FLOW_GUESS_BINDING);
        }

        m_buffer.bind(DATA_BINDING);
}

//

FlowImagesMemory::FlowImagesMemory(const opengl::TextureR32F& image_dx, const opengl::TextureR32F& image_dy,
                                   const opengl::TextureR32F& image_i, const opengl::TextureR32F& texture_j)
        : m_buffer(sizeof(Images))
{
        Images images;

        images.image_dx = image_dx.image_resident_handle_read_only();
        images.image_dy = image_dy.image_resident_handle_read_only();
        images.image_i = image_i.image_resident_handle_read_only();
        images.texture_j = texture_j.texture().texture_resident_handle();

        m_buffer.copy(images);
}

void FlowImagesMemory::bind() const
{
        m_buffer.bind(IMAGES_BINDING);
}
}
