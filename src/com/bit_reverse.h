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

#if 0

#include "bits.h"
#include "error.h"

#include <cstdint>
#include <vector>

namespace ns
{
constexpr int bit_reverse(int bin_size, int v)
{
        int r = 0;
        for (int b = 0; b < bin_size; ++b)
        {
                r <<= 1;
                r |= (v & 0x1);
                v >>= 1;
        }
        return r;
}

uint8_t bit_reverse_8(uint8_t v);
uint16_t bit_reverse_16(uint16_t v);
uint32_t bit_reverse_32(uint32_t v);
uint64_t bit_reverse_64(uint64_t v);

void create_bit_reverse_lookup_table(int N, std::vector<int>* data);

template <typename T>
void bit_reverse(const std::vector<int>& reverse_lookup, std::vector<T>* data)
{
        int N = data->size();
        if (N != static_cast<int>(reverse_lookup.size()))
        {
                error("bit reverse size error");
        }
        for (int i = 0; i < N; ++i)
        {
                int r = reverse_lookup[i];
                if (i < r) // для исключения одинаковых обменов и уже сделанных обменов
                {
                        std::swap((*data)[i], (*data)[r]);
                }
        }
}

template <typename T>
void bit_reverse(std::vector<T>* data)
{
        int N = data->size();
        int bin_size = binary_size(N);

        for (int i = 0; i < N; ++i)
        {
                int r = bit_reverse(bin_size, i);
                if (i < r) // для исключения одинаковых обменов и уже сделанных обменов
                {
                        std::swap((*data)[i], (*data)[r]);
                }
        }
}
}

#endif
