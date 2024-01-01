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

#include "performance.h"

#include "../simplex_noise.h"

#include <src/settings/dimensions.h>
#include <src/test/test.h>

#include <cstddef>
#include <string_view>
#include <utility>

namespace ns::noise::test
{
namespace
{
constexpr std::string_view NAME = "Simplex Noise";

template <std::size_t N, typename T>
void test()
{
        test_performance(NAME, simplex_noise<N, T>);
}

template <std::size_t... I>
void test(std::index_sequence<I...>&&)
{
        ((test<I, float>()), ...);
        ((test<I, double>()), ...);
}

void test_performance()
{
        test(settings::Dimensions2());
}

TEST_PERFORMANCE("Simplex Noise", test_performance)
}
}
