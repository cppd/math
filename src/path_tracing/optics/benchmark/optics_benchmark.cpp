/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "optics_benchmark.h"

#include "com/log.h"
#include "com/random/engine.h"
#include "com/time.h"
#include "path_tracing/optics/optics.h"

#include <random>
#include <vector>

void optics_benchmark()
{
        constexpr int COUNT = 10000000;

        RandomEngineWithSeed<std::mt19937_64> engine;
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        std::vector<vec3> data;
        for (int i = 0; i < COUNT; ++i)
        {
                data.push_back(normalize(vec3(urd(engine), urd(engine), urd(engine))));
        }

        vec3 normal = normalize(vec3(0.1, -0.2, 0.3));

        double eta = 1.0 / 1.5;

        {
                double time = time_in_seconds();
                vec3 t;
                double sum = 0;
                for (const vec3& v : data)
                {
                        if (refract(v, normal, eta, &t))
                        {
                                sum += std::abs(t[0]) + std::abs(t[1]) + std::abs(t[2]);
                        }
                }
                LOG("refract  : " + to_string(time_in_seconds() - time) + ", sum = " + to_string(sum));
        }

        {
                double time = time_in_seconds();
                vec3 t;
                double sum = 0;
                for (const vec3& v : data)
                {
                        if (refract2(v, normal, eta, &t))
                        {
                                sum += std::abs(t[0]) + std::abs(t[1]) + std::abs(t[2]);
                        }
                }
                LOG("refract 2: " + to_string(time_in_seconds() - time) + ", sum = " + to_string(sum));
        }
}
