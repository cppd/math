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

#pragma once

#include "error.h"
#include "log.h"
#include "print.h"

#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace ns
{
class MemoryArena final
{
        static constexpr std::size_t ALIGN = alignof(std::max_align_t);
        static constexpr std::size_t BLOCK_SIZE = 1 << 18;

        static constexpr std::size_t MAX_OBJECT_SIZE = BLOCK_SIZE / 1024;
        static constexpr std::size_t BLOCK_COUNT_WITHOUT_WARNING = 8;

        class Block final
        {
                struct AlignedStorage final
                {
                        alignas(ALIGN) std::byte data[ALIGN];
                };

                std::vector<AlignedStorage> data_;

        public:
                Block()
                {
                        static_assert(ALIGN <= BLOCK_SIZE);
                        static_assert(BLOCK_SIZE % ALIGN == 0);
                        static_assert(sizeof(decltype(data_)::value_type) == ALIGN);
                        static_assert(alignof(decltype(data_)::value_type) == ALIGN);

                        data_.resize(BLOCK_SIZE / ALIGN);
                }

                std::byte* data(const std::size_t index)
                {
                        return reinterpret_cast<std::byte*>(data_.data()) + index;
                }
        };

        template <typename T>
        static std::size_t next_index(const std::size_t index)
        {
                // (index + alignof(T) - 1) & (~(alignof(T) - 1));
                return ((index + alignof(T) - 1) / alignof(T)) * alignof(T);
        }

        template <class T, class... Args>
        static T* create_object(Block* const block, const std::size_t index, Args&&... args)
        {
                return new (block->data(index)) T(std::forward<Args>(args)...);
        }

        const std::thread::id thread_id_ = std::this_thread::get_id();

        std::vector<std::unique_ptr<Block>> blocks_;
        std::size_t block_;
        std::size_t index_;

        MemoryArena()
        {
                blocks_.push_back(std::make_unique<Block>());
                clear();
        }

public:
        static MemoryArena& thread_local_instance()
        {
                static thread_local MemoryArena memory_arena;
                return memory_arena;
        }

        [[nodiscard]] static constexpr std::size_t block_size()
        {
                return BLOCK_SIZE;
        }

        [[nodiscard]] std::size_t used_blocks() const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                return block_ + 1;
        }

        [[nodiscard]] std::size_t used_bytes() const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                return block_ * BLOCK_SIZE + index_;
        }

        void clear()
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                block_ = 0;
                index_ = 0;
        }

        template <class T, class... Args>
        [[nodiscard]] T* make(Args&&... args)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                static_assert(std::is_trivially_destructible_v<T>);
                static_assert(alignof(T) <= ALIGN);
                static_assert(ALIGN % alignof(T) == 0);

                static_assert(sizeof(T) <= MAX_OBJECT_SIZE);

                const std::size_t index = next_index<T>(index_);
                if (index + sizeof(T) <= BLOCK_SIZE)
                {
                        auto* const ptr = create_object<T>(blocks_[block_].get(), index, std::forward<Args>(args)...);
                        index_ = index + sizeof(T);
                        return ptr;
                }

                if (block_ + 1 == blocks_.size())
                {
                        blocks_.push_back(std::make_unique<Block>());
                }
                auto* const ptr = create_object<T>(blocks_[block_ + 1].get(), 0, std::forward<Args>(args)...);
                ++block_;
                index_ = sizeof(T);

                if (used_blocks() > BLOCK_COUNT_WITHOUT_WARNING)
                {
                        LOG("MemoryArena has too many blocks; used blocks = " + to_string(used_blocks())
                            + ", used bytes = " + to_string(used_bytes()));
                }

                return ptr;
        }
};

template <class T, class... Args>
[[nodiscard]] T* make_arena_ptr(Args&&... args)
{
        return MemoryArena::thread_local_instance().make<T>(std::forward<Args>(args)...);
}
}
