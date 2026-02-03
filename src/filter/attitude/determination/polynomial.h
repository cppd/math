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

#pragma once

#include <array>
#include <string>

namespace ns::filter::attitude::determination
{
template <typename T>
class CharacteristicPolynomial final
{
        // x^4 + c0 * x^2 + c1 * x + c2
        const std::array<T, 3> f_;

        // 4 * x^3 + c0 * x + c1
        const std::array<T, 2> d_;

public:
        explicit CharacteristicPolynomial(const std::array<T, 3>& f)
                : f_(f),
                  d_{2 * f[0], f[1]}
        {
        }

        [[nodiscard]] T f(const T x) const
        {
                return ((x * x + f_[0]) * x + f_[1]) * x + f_[2];
        }

        [[nodiscard]] T d(const T x) const
        {
                return (4 * x * x + d_[0]) * x + d_[1];
        }

        [[nodiscard]] std::string str() const;
};
}
