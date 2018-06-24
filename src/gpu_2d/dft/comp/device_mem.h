/*
Copyright (C) 2017, 2018 Topological Manifold

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

#pragma once

#include "graphics/opengl/objects.h"

#include <vector>

enum class MemoryUsage
{
        StaticCopy,
        DynamicCopy
};

template <typename T>
class DeviceMemory final
{
        size_t m_size;
        MemoryUsage m_usage;
        ShaderStorageBuffer m_buffer;

public:
        DeviceMemory(int size, MemoryUsage usage) : m_size(size), m_usage(usage)
        {
                switch (m_usage)
                {
                case MemoryUsage::StaticCopy:
                        m_buffer.create_static_copy(size * sizeof(T));
                        break;
                case MemoryUsage::DynamicCopy:
                        m_buffer.create_dynamic_copy(size * sizeof(T));
                        break;
                }
        }

        void load(const std::vector<T>& data) const
        {
                if (data.size() != m_size)
                {
                        error("Storage size error");
                }
                switch (m_usage)
                {
                case MemoryUsage::StaticCopy:
                        m_buffer.load_static_copy(data);
                        break;
                case MemoryUsage::DynamicCopy:
                        m_buffer.load_dynamic_copy(data);
                        break;
                }
        }

        void read(std::vector<T>* data) const
        {
                if (data->size() != m_size)
                {
                        error("Storage size error");
                }
                m_buffer.read(data);
        }

        std::vector<T> read() const
        {
                std::vector<T> v(size());
                read(&v);
                return v;
        }

        void bind(int point) const noexcept
        {
                m_buffer.bind(point);
        }

        int size() const noexcept
        {
                return m_size;
        }
};
