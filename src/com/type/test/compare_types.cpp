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

#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <gmpxx.h>
#include <vector>

namespace ns
{
namespace
{
constexpr int N = 1 << 27;

template <typename T>
__attribute__((noinline)) double computation(std::vector<T>* v)
{
        constexpr T ADD = 20;
        constexpr T SUB = 30;

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < N; ++i)
        {
                (*v)[i] = ((*v)[i] + ADD) * ((*v)[i] - SUB) + ADD;
        }
        return duration_from(start_time);
}

__attribute__((noinline)) double computation(std::vector<mpz_class>* v)
{
        const mpz_class add = 20;
        const mpz_class sub = 30;
        mpz_class tmp1;
        mpz_class tmp2;

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < N; ++i)
        {
                mpz_add(tmp1.get_mpz_t(), (*v)[i].get_mpz_t(), add.get_mpz_t());
                mpz_sub(tmp2.get_mpz_t(), (*v)[i].get_mpz_t(), sub.get_mpz_t());
                mpz_mul((*v)[i].get_mpz_t(), tmp1.get_mpz_t(), tmp2.get_mpz_t());
                mpz_add((*v)[i].get_mpz_t(), (*v)[i].get_mpz_t(), add.get_mpz_t());
        }
        return duration_from(start_time);
}

__attribute__((noinline)) double computation(std::vector<mpf_class>* v)
{
        const mpf_class add = 20;
        const mpf_class sub = 30;
        mpf_class tmp1;
        mpf_class tmp2;

        Clock::time_point start_time = Clock::now();
        for (int i = 0; i < N; ++i)
        {
                mpf_add(tmp1.get_mpf_t(), (*v)[i].get_mpf_t(), add.get_mpf_t());
                mpf_sub(tmp2.get_mpf_t(), (*v)[i].get_mpf_t(), sub.get_mpf_t());
                mpf_mul((*v)[i].get_mpf_t(), tmp1.get_mpf_t(), tmp2.get_mpf_t());
                mpf_add((*v)[i].get_mpf_t(), (*v)[i].get_mpf_t(), add.get_mpf_t());
        }
        return duration_from(start_time);
}

void compare_types()
{
        {
                std::vector<mpz_class> v(N, 1e16);
                LOG("MPZ " + to_string(computation(&v)));
        }
        {
                mpf_set_default_prec(128);
                std::vector<mpf_class> v(N, 1e12);
                LOG("MPF " + to_string(computation(&v)));
        }
        {
                std::vector<__float128> v(N, 1e12);
                LOG("__float128 " + to_string(computation(&v)));
        }
        {
                std::vector<float> v(N, 1e6);
                LOG("float " + to_string(computation(&v)));
        }
        {
                std::vector<double> v(N, 1e12);
                LOG("double " + to_string(computation(&v)));
        }
        {
                std::vector<long double> v(N, 1e12);
                LOG("long double " + to_string(computation(&v)));
        }
        {
                std::vector<int> v(N, std::sqrt(Limits<int>::max()) / 10);
                LOG("int " + to_string(computation(&v)));
        }
        {
                std::vector<long> v(N, std::sqrt(Limits<long>::max()) / 10);
                LOG("long " + to_string(computation(&v)));
        }
        {
                std::vector<long long> v(N, std::sqrt(Limits<long long>::max()) / 10);
                LOG("long long " + to_string(computation(&v)));
        }
        {
                std::vector<__int128> v(N, 1e16);
                LOG("__int128 " + to_string(computation(&v)));
        }
        {
                std::vector<unsigned __int128> v(N, 1e16);
                LOG("unsigned __int128 " + to_string(computation(&v)));
        }
}

TEST_PERFORMANCE("Compare arithmetic types", compare_types)
}
}
