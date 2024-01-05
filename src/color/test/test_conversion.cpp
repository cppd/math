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

#include <src/color/conversion.h>
#include <src/com/math.h>
#include <src/numerical/vector.h>

namespace ns::color
{
namespace
{
template <typename T>
constexpr bool compare(const Vector<3, T>& a, const Vector<3, T>& b, const T& precision)
{
        for (int i = 0; i < 3; ++i)
        {
                if (!(absolute(a[i] - b[i]) < precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
constexpr bool check_1(const Vector<3, T>& v, const T& precision)
{
        const Vector<3, T> rgb = xyz_to_linear_srgb<T>(v[0], v[1], v[2]);
        const Vector<3, T> xyz = linear_srgb_to_xyz<T>(rgb[0], rgb[1], rgb[2]);
        return compare<T>(v, xyz, precision);
}

template <typename T>
constexpr bool check_2(const Vector<3, T>& v, const T& precision)
{
        const Vector<3, T> xyz = linear_srgb_to_xyz<T>(v[0], v[1], v[2]);
        const Vector<3, T> rgb = xyz_to_linear_srgb<T>(xyz[0], xyz[1], xyz[2]);
        return compare<T>(v, rgb, precision);
}

template <int I, int MAX, typename T>
constexpr bool check(Vector<3, T>& v, const T& precision)
{
        static_assert(I >= 0 && I <= 3);
        if constexpr (I == 3)
        {
                return check_1<T>(v, precision) && check_2<T>(v, precision);
        }
        else
        {
                for (int i = 0; i <= MAX; ++i)
                {
                        v[I] = static_cast<T>(i) / MAX;
                        if (!check<I + 1, MAX, T>(v, precision))
                        {
                                return false;
                        }
                }
                return true;
        }
}

template <typename T>
constexpr bool check(const T& precision)
{
        constexpr int MAX = 4;
        Vector<3, T> v;
        return check<0, MAX, T>(v, precision);
}

template <typename T>
struct Check final
{
        static constexpr T D65_X = 0.9505;
        static constexpr T D65_Y = 1;
        static constexpr T D65_Z = 1.089;
        static_assert(compare<T>(xyz_to_linear_srgb<T>(D65_X, D65_Y, D65_Z), {1, 1, 1}, 1e-6));
        static_assert(compare<T>(linear_srgb_to_xyz<T>(1, 1, 1), {D65_X, D65_Y, D65_Z}, 1e-16));

        static_assert(check<T>(1e-6));
};

template struct Check<float>;
template struct Check<double>;
template struct Check<long double>;
}
}
