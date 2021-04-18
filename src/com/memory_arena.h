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

#pragma once

#include "error.h"

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace ns
{
class MemoryArena
{
        static constexpr std::size_t ALIGN = alignof(std::max_align_t);
        static constexpr std::size_t BLOCK_SIZE = 1 << 20;

        class Block final
        {
                std::vector<std::aligned_storage_t<ALIGN, ALIGN>> m_data;

        public:
                Block()
                {
                        static_assert(ALIGN <= BLOCK_SIZE);
                        static_assert(BLOCK_SIZE % ALIGN == 0);
                        static_assert(sizeof(decltype(m_data)::value_type) == ALIGN);
                        static_assert(alignof(decltype(m_data)::value_type) == ALIGN);

                        m_data.resize(BLOCK_SIZE / ALIGN);
                }

                std::byte* data(std::size_t index)
                {
                        return reinterpret_cast<std::byte*>(m_data.data()) + index;
                }
        };

        template <typename T>
        static std::size_t next_index(std::size_t index)
        {
                // (index + alignof(T) - 1) & (~(alignof(T) - 1));
                return ((index + alignof(T) - 1) / alignof(T)) * alignof(T);
        }

        template <class T, class... Args>
        static T* create_object(Block* block, std::size_t index, Args&&... args)
        {
                return new (block->data(index)) T(std::forward<Args>(args)...);
        }

        const std::thread::id m_thread_id = std::this_thread::get_id();
        std::vector<std::unique_ptr<Block>> m_blocks;
        std::size_t m_block;
        std::size_t m_index;

        MemoryArena()
        {
                m_blocks.push_back(std::make_unique<Block>());
                clear();
        }

public:
        static MemoryArena& thread_local_instance()
        {
                static thread_local MemoryArena memory_arena;
                return memory_arena;
        }

        static constexpr std::size_t block_size()
        {
                return BLOCK_SIZE;
        }

        std::size_t used_blocks() const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_block + 1;
        }

        std::size_t used_bytes() const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_block * BLOCK_SIZE + m_index;
        }

        void clear()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_block = 0;
                m_index = 0;
        }

        template <class T, class... Args>
        T* make(Args&&... args)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                static_assert(std::is_trivially_destructible_v<T>);
                static_assert(alignof(T) <= ALIGN);
                static_assert(ALIGN % alignof(T) == 0);

                static_assert(sizeof(T) <= BLOCK_SIZE / 1024);

                std::size_t index = next_index<T>(m_index);
                if (index + sizeof(T) <= BLOCK_SIZE)
                {
                        T* ptr = create_object<T>(m_blocks[m_block].get(), index, std::forward<Args>(args)...);
                        m_index = index + sizeof(T);
                        return ptr;
                }

                if (m_block + 1 == m_blocks.size())
                {
                        m_blocks.push_back(std::make_unique<Block>());
                }
                T* ptr = create_object<T>(m_blocks[m_block + 1].get(), 0, std::forward<Args>(args)...);
                ++m_block;
                m_index = sizeof(T);
                return ptr;
        }
};

template <class T, class... Args>
T* make_arena_ptr(Args&&... args)
{
        return MemoryArena::thread_local_instance().make<T>(std::forward<Args>(args)...);
}
}
