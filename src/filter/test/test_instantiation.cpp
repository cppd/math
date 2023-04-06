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

#include "../filter.h"

namespace ns::filter
{
namespace
{
template <std::size_t N, std::size_t M, typename T>
struct TestInstantiation final
{
        void test()
        {
                Filter<N, M, T> f;
                f.set_x(Vector<N, T>{});
                f.set_p(Matrix<N, N, T>{});
                f.set_f(Matrix<N, N, T>{});
                f.set_q(Matrix<N, N, T>{});
                f.set_h(Matrix<M, N, T>{});
                f.set_r(Matrix<M, M, T>{});
                static_cast<void>(f.x());
                static_cast<void>(f.p());
                f.predict();
                f.update(Vector<M, T>{});
        }
};

#define TEST_INSTANTIATION_N_M(N, M)                               \
        template void TestInstantiation<(N), (M), float>::test();  \
        template void TestInstantiation<(N), (M), double>::test(); \
        template void TestInstantiation<(N), (M), long double>::test();

#define TEST_INSTANTIATION_N(N)        \
        TEST_INSTANTIATION_N_M((N), 1) \
        TEST_INSTANTIATION_N_M((N), 2) \
        TEST_INSTANTIATION_N_M((N), 3)

TEST_INSTANTIATION_N(1)
TEST_INSTANTIATION_N(2)
TEST_INSTANTIATION_N(3)
}
}
