/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/bit/reverse.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <numeric>
#include <vector>

namespace ns
{
namespace
{
static_assert(bit_reverse(4, 0b1011) == 0b1101);
static_assert(bit_reverse(31, 0b1011001110001111000011111000001) == 0b1000001111100001111000111001101);
static_assert(bit_reverse_8(0b10110011) == 0b11001101);
static_assert(bit_reverse(8, 0b10110011) == 0b11001101);
static_assert(bit_reverse_16(0b1011001110001111) == 0b1111000111001101);
static_assert(bit_reverse(16, 0b1011001110001111) == 0b1111000111001101);
static_assert(bit_reverse_32(0b10110011100011110000111110000011) == 0b11000001111100001111000111001101);
static_assert(bit_reverse(32, 0b10110011100011110000111110000011) == 0b11000001111100001111000111001101);
static_assert(
        bit_reverse_64(0b1011001110001111000011111000001111110000001111111000000011111111)
        == 0b1111111100000001111111000000111111000001111100001111000111001101);
static_assert(
        bit_reverse(64, 0b1011001110001111000011111000001111110000001111111000000011111111)
        == 0b1111111100000001111111000000111111000001111100001111000111001101);

template <typename T>
void test(const int size, const std::vector<T>& reversed)
{
        std::vector<T> data(size);
        std::iota(data.begin(), data.end(), 0);

        const BitReverse bit_reverse(size);
        bit_reverse.reverse(&data);

        if (!(data == reversed))
        {
                error("Bit-reversal permutation " + to_string(data) + " are not equal to " + to_string(reversed));
        }
}

template <typename T>
void test()
{
        test<T>(1, {0});
        test<T>(2, {0, 1});
        test<T>(4, {0, 2, 1, 3});
        test<T>(8, {0, 4, 2, 6, 1, 5, 3, 7});
        test<T>(16, {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15});
}

void test_bit_reverse()
{
        test<int>();
        test<unsigned int>();
        test<long long>();
        test<unsigned long long>();
        test<float>();
        test<double>();
        test<long double>();
}

TEST_SMALL("Bit Reverse", test_bit_reverse)
}
}
