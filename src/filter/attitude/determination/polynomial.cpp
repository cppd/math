/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "polynomial.h"

#include <src/com/print.h>

#include <cmath>
#include <string>

namespace ns::filter::attitude::determination
{
namespace
{
template <typename T>
auto to_string_sign(const T v)
{
        return (v < 0 ? " - " : " + ") + to_string(std::abs(v));
}
}

template <typename T>
std::string CharacteristicPolynomial<T>::str() const
{
        std::string res;

        res += "f = x^4";
        if (!(f_[0] == 0))
        {
                res += to_string_sign(f_[0]) + " * x^2";
        }
        if (!(f_[1] == 0))
        {
                res += to_string_sign(f_[1]) + " * x";
        }
        if (!(f_[2] == 0))
        {
                res += to_string_sign(f_[2]);
        }

        res += ", ";
        res += "d = 4 * x^3";
        if (!(d_[0] == 0))
        {
                res += to_string_sign(d_[0]) + " * x";
        }
        if (!(d_[1] == 0))
        {
                res += to_string_sign(d_[1]);
        }

        return res;
}

#define TEMPLATE(T) template class CharacteristicPolynomial<T>;

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
