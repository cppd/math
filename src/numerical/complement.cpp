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

#include "complement.h"

namespace ns::numerical
{
#if 0
void add_mul(mpz_class* r, const mpz_class& a, const mpz_class& b, const mpz_class& c, const mpz_class& d,
                          const mpz_class& e)
{
        mpz_mul(r->get_mpz_t(), b.get_mpz_t(), c.get_mpz_t());
        mpz_submul(r->get_mpz_t(), d.get_mpz_t(), e.get_mpz_t());
        mpz_mul(r->get_mpz_t(), a.get_mpz_t(), r->get_mpz_t());
}

void to_res(mpz_class* res, const mpz_class& a, const mpz_class& b, const mpz_class& c)
{
        *res = a;
        mpz_sub(res->get_mpz_t(), res->get_mpz_t(), b.get_mpz_t());
        mpz_add(res->get_mpz_t(), res->get_mpz_t(), c.get_mpz_t());
}

Vector<4, mpz_class> orthogonal_complement(const std::array<Vector<4, mpz_class>, 3>& v)
{
        thread_local Vector<4, mpz_class> res;

        thread_local mpz_class x1;
        thread_local mpz_class x2;
        thread_local mpz_class x3;

        add_mul(&x1, v[0][1], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul(&x2, v[0][2], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul(&x3, v[0][3], v[1][1], v[2][2], v[1][2], v[2][1]);
        to_res(&res[0], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][2], v[2][3], v[1][3], v[2][2]);
        add_mul(&x2, v[0][2], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul(&x3, v[0][3], v[1][0], v[2][2], v[1][2], v[2][0]);
        to_res(&res[1], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][1], v[2][3], v[1][3], v[2][1]);
        add_mul(&x2, v[0][1], v[1][0], v[2][3], v[1][3], v[2][0]);
        add_mul(&x3, v[0][3], v[1][0], v[2][1], v[1][1], v[2][0]);
        to_res(&res[2], x1, x2, x3);

        add_mul(&x1, v[0][0], v[1][1], v[2][2], v[1][2], v[2][1]);
        add_mul(&x2, v[0][1], v[1][0], v[2][2], v[1][2], v[2][0]);
        add_mul(&x3, v[0][2], v[1][0], v[2][1], v[1][1], v[2][0]);
        to_res(&res[3], x1, x2, x3);

        mpz_neg(res[1].get_mpz_t(), res[1].get_mpz_t());
        mpz_neg(res[3].get_mpz_t(), res[3].get_mpz_t());

        return res;
}
#endif
}
