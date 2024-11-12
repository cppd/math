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

#include <src/filter/attitude/fusion.h>

namespace ns::filter::attitude
{
namespace
{
template <typename T>
struct Test final
{
        void test()
        {
                Fusion<T> f;
                f.update_gyro({0, 0, 0}, 0, 0);
                f.update_acc({0, 0, 0});
                static_cast<void>(f.attitude());
        }
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;
}
}
