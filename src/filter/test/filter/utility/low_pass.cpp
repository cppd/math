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

#include "low_pass.h"

#include <src/com/error.h>

#include <array>

namespace ns::filter::test::filter::utility
{
namespace
{
// Frequency = 2 * PI / 50
template <typename T>
constexpr std::array FIR = std::to_array<T>(
        {0.000701978170083000407L, 0.001574484474175391390L, 0.002623125599757595452L, 0.003849929201563881054L,
         0.005253060335008797104L, 0.006826628597452609868L, 0.008560594977005377896L, 0.010440784712724792631L,
         0.012449009511619062138L, 0.014563299335531457533L, 0.016758240760688322581L, 0.019005415724495402008L,
         0.021273931408979845922L, 0.023531029166520587504L, 0.025742757863828264717L, 0.027874694888250594044L,
         0.02989269639833219064L,  0.03176365726581736731L,  0.03345626059031107746L,  0.03494169669415914545L,
         0.03619433212860469303L,  0.03719231042874077937L,  0.03791806811135900109L,  0.03835875166594275124L,
         0.03850652397809602430L,  0.03835875166594275124L,  0.03791806811135900109L,  0.03719231042874077937L,
         0.03619433212860469303L,  0.03494169669415914545L,  0.03345626059031107746L,  0.03176365726581736731L,
         0.02989269639833219064L,  0.027874694888250594044L, 0.025742757863828264717L, 0.023531029166520587504L,
         0.021273931408979845922L, 0.019005415724495402008L, 0.016758240760688322581L, 0.014563299335531457533L,
         0.012449009511619062138L, 0.010440784712724792631L, 0.008560594977005377896L, 0.006826628597452609868L,
         0.005253060335008797104L, 0.003849929201563881054L, 0.002623125599757595452L, 0.001574484474175391390L,
         0.000701978170083000407L});

static_assert(FIR<double>.size() == 49);
}

template <typename T>
void LowPassFilter<T>::clear()
{
        values_.clear();
}

template <typename T>
void LowPassFilter<T>::push(const T value)
{
        while (values_.size() >= FIR<T>.size())
        {
                values_.pop_front();
        }
        values_.push_back(value);
}

template <typename T>
std::optional<T> LowPassFilter<T>::value() const
{
        if (values_.size() < FIR<T>.size())
        {
                return std::nullopt;
        }

        auto iter = values_.cend() - FIR<T>.size();
        T sum = 0;
        for (std::size_t i = 0; i < FIR<T>.size(); ++i, ++iter)
        {
                sum += *iter * FIR<T>[i];
        }
        return sum;
}

#define TEMPLATE(T) template class LowPassFilter<T>;

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
