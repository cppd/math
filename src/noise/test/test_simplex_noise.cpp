/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "image.h"

#include "../simplex_noise.h"

#include <src/com/log.h>
#include <src/test/test.h>

namespace ns::noise::test
{
namespace
{
constexpr std::string_view FILE_NAME = "simplex_noise";
constexpr int IMAGE_SIZE = 500;
constexpr int NOISE_SIZE = 10;

template <std::size_t N, typename T>
void test()
{
        make_noise_image(FILE_NAME, IMAGE_SIZE, NOISE_SIZE, simplex_noise<N, T>);
}

void test()
{
        test<2, float>();
        test<2, double>();
}

void test_noise()
{
        LOG("Test simplex noise");
        test();
        LOG("Test simplex noise passed");
}

TEST_SMALL("Simplex Noise", test_noise)
}
}
