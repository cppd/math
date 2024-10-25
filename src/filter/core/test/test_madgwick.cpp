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

#include <src/filter/core/madgwick.h>
#include <src/numerical/quaternion.h>

namespace ns::filter::core::test
{
template <typename T>
struct Test final
{
        T beta()
        {
                return madgwick_beta<T>(0);
        }

        numerical::Quaternion<T> f1()
        {
                Madgwick<T> m;
                return m.update({0, 0, 0}, {0, 0, 0}, 0, 0, 0);
        }

        numerical::Quaternion<T> f2()
        {
                MadgwickMarg<T> m;
                return m.update({0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0, 0, 0, 0, 0);
        }
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;
}
