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

#include <src/com/error.h>
#include <src/com/primes.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace ns
{
namespace
{
static_assert(!PRIMES.empty());

std::vector<bool> sieve_of_eratosthenes(const unsigned max)
{
        const unsigned m_sqrt = std::floor(std::sqrt(static_cast<double>(max)));

        std::vector<bool> data(max + 1, true);
        data[0] = false;
        data[1] = false;

        unsigned i = 2;
        while (true)
        {
                while (i <= m_sqrt && !data[i])
                {
                        ++i;
                }
                if (i > m_sqrt)
                {
                        break;
                }
                for (unsigned j = i * i; j <= max; j += i)
                {
                        data[j] = false;
                }
                ++i;
        }

        return data;
}

void test()
{
        const std::vector<bool> data = sieve_of_eratosthenes(PRIMES.back());

        std::size_t i = 0;
        for (std::size_t j = 2; j < data.size(); ++j)
        {
                if (!data[j])
                {
                        continue;
                }
                if (i >= PRIMES.size())
                {
                        error("prime index " + to_string(i) + " is out of bounds [0, " + to_string(PRIMES.size())
                              + ")");
                }
                if (!(PRIMES[i] == j))
                {
                        error("prime index " + to_string(i) + "; prime " + to_string(PRIMES[i]) + " is not equal to "
                              + to_string(j));
                }
                ++i;
        }
        if (i != PRIMES.size())
        {
                error("prime index = " + to_string(i) + "; prime count = " + to_string(PRIMES.size()));
        }
}

TEST_SMALL("Primes", test)
}
}
