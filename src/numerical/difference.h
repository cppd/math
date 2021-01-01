/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "vec.h"

#include <src/com/mpz.h>

namespace ns::numerical
{
template <std::size_t N, typename T, typename ResultType>
void difference(Vector<N, ResultType>* result, const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                (*result)[i] = a[i] - b[i];
        }
}

template <std::size_t N, typename T>
void difference(Vector<N, mpz_class>* result, const Vector<N, T>& a, const Vector<N, T>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                mpz_from_any(&(*result)[i], a[i] - b[i]);
        }
}

template <std::size_t N>
void difference(Vector<N, mpz_class>* result, const Vector<N, mpz_class>& a, const Vector<N, mpz_class>& b)
{
        for (unsigned i = 0; i < N; ++i)
        {
                mpz_sub((*result)[i]->get_mpz_t(), a[i].get_mpz_t(), b[i].get_mpz_t());
        }
}
}
