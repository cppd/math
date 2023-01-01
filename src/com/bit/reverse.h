/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../error.h"

#include <array>
#include <bit>
#include <cstdint>
#include <vector>

namespace ns
{
namespace bit_reverse_implementation
{
// clang-format off
inline constexpr std::array<std::uint8_t, 256> BIT_REVERSE_TABLE =
{
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};
// clang-format on
}

template <typename T>
constexpr T bit_reverse(const int bit_count, T v)
{
        static_assert(std::is_integral_v<T>);
        T r = 0;
        for (int b = 0; b < bit_count; ++b)
        {
                r <<= 1;
                r |= (v & 0x1);
                v >>= 1;
        }
        return r;
}

constexpr std::uint8_t bit_reverse_8(const std::uint8_t v)
{
        namespace impl = bit_reverse_implementation;
        return impl::BIT_REVERSE_TABLE[v];
}

constexpr std::uint16_t bit_reverse_16(const std::uint16_t v)
{
        namespace impl = bit_reverse_implementation;
        return (static_cast<std::uint16_t>(impl::BIT_REVERSE_TABLE[v & 0xff]) << 8)
               | (static_cast<std::uint16_t>(impl::BIT_REVERSE_TABLE[(v >> 8) & 0xff]));
}

constexpr std::uint32_t bit_reverse_32(const std::uint32_t v)
{
        namespace impl = bit_reverse_implementation;
        return (static_cast<std::uint32_t>(impl::BIT_REVERSE_TABLE[v & 0xff]) << 24)
               | (static_cast<std::uint32_t>(impl::BIT_REVERSE_TABLE[(v >> 8) & 0xff]) << 16)
               | (static_cast<std::uint32_t>(impl::BIT_REVERSE_TABLE[(v >> 16) & 0xff]) << 8)
               | (static_cast<std::uint32_t>(impl::BIT_REVERSE_TABLE[(v >> 24) & 0xff]));
}

constexpr std::uint64_t bit_reverse_64(const std::uint64_t v)
{
        namespace impl = bit_reverse_implementation;
        return (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[v & 0xff]) << 56)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 8) & 0xff]) << 48)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 16) & 0xff]) << 40)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 24) & 0xff]) << 32)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 32) & 0xff]) << 24)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 40) & 0xff]) << 16)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 48) & 0xff]) << 8)
               | (static_cast<std::uint64_t>(impl::BIT_REVERSE_TABLE[(v >> 56) & 0xff]));
}

class BitReverse
{
        static std::vector<int> create_table(const int size)
        {
                if (size <= 0)
                {
                        error("Bit-reversal size " + std::to_string(size) + " is not positive");
                }

                if (!std::has_single_bit(static_cast<unsigned>(size)))
                {
                        error("Bit-reversal size " + std::to_string(size) + " is not an integral power of 2");
                }

                const int bit_count = std::bit_width(static_cast<unsigned>(size)) - 1;
                std::vector<int> res(size);
                for (int i = 0; i < size; ++i)
                {
                        res[i] = bit_reverse(bit_count, i);
                }
                return res;
        }

        std::vector<int> table_;

public:
        explicit BitReverse(const int size)
                : table_(create_table(size))
        {
        }

        template <typename T>
        void reverse(std::vector<T>* const data) const
        {
                if (data->size() != table_.size())
                {
                        error("Bit-reversal data size " + std::to_string(data->size()) + " is not equal to "
                              + std::to_string(table_.size()));
                }

                const int size = data->size();
                for (int i = 0; i < size; ++i)
                {
                        const int r = table_[i];
                        if (i < r)
                        {
                                std::swap((*data)[i], (*data)[r]);
                        }
                }
        }
};
}
