/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "sphere_uniform.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <type_traits>
#include <unordered_map>

namespace ns::sampling
{
/*
angle = ∠(vector, normal)
PDF = cos(angle)^n * sin(angle)^p
n >= 1
p = N-2 >= 1
0 <= angle <= PI/2

PDF'(x) = 0
x = atan(sqrt((p/n))

The rejection method

mean=ArcTan[Sqrt[p/n]];
deviation=1/Sqrt[(n+p)*Sqrt[2]];
c=(deviation*Sqrt[2*Pi])*(Cos[mean]^n*Sin[mean]^p);

PDF <= c * PDF(normal_distribution(mean, deviation))
n = 100, p = 2, efficiency ≈ 80%

Plot[{Cos[x]^n*Sin[x]^p,c*PDF[NormalDistribution[mean,deviation],x]},
  {x,-Pi/8,Pi/2},Filling->Axis,PlotRange->Full,PlotLegends->"Expressions"]
N[Integrate[Cos[x]^n*Sin[x]^p/c,{x,0,Pi/2}]]
*/

/*
3-space only

angle = ∠(vector, normal)
PDF = cos(angle)^n * sin(angle)
0 <= angle <= PI/2

d = Assuming[n >= 0,
  ProbabilityDistribution[(Cos[x]^n) * Sin[x], {x, 0, Pi/2}, Method -> "Normalize"]];
PDF[d, x]
CDF[d, x]

CDF = 1 - cos(angle)^(1 + n)
Inverse CDF = acos((1 - CDF)^(1 / (1 + n)))
Inverse CDF = acos(x^(1 / (1 + n)))
Projection on normal = cos(acos(x^(1 / (1 + n))))
Projection on normal = x^(1 / (1 + n))

uniform x = length_of_random_vector_in_2_sphere ^ 2
Projection on normal = squared_length_of_random_vector_in_2_sphere ^ (1 / (1 + n))
*/

namespace sphere_power_cosine_implementation
{
template <std::size_t N, typename T>
class PowerCosineOnHemisphere final
{
        static_assert(N > 3);

        T n_;
        T p_;
        T mean_;
        T normal_distribution_coef_;
        std::normal_distribution<T> normal_distribution_;
        std::uniform_real_distribution<T> urd_;

        explicit PowerCosineOnHemisphere(const std::type_identity_t<T> power)
        {
                if (!(power >= 1))
                {
                        error("Power for cosine " + to_string(power) + " must be greater than or equal to 1");
                }

                n_ = power;
                p_ = N - 2;
                mean_ = std::atan(std::sqrt(p_ / n_));
                const T deviation = T{1} / std::sqrt((n_ + p_) * std::sqrt(T{2}));
                normal_distribution_ = std::normal_distribution<T>(mean_, deviation);
                normal_distribution_coef_ = T{-1} / (2 * square(deviation));
                const T max = std::pow(std::cos(mean_), n_) * std::pow(std::sin(mean_), p_);
                urd_ = std::uniform_real_distribution<T>(0, max);
        }

public:
        template <typename RandomEngine>
        numerical::Vector<N, T> sample(RandomEngine& engine)
        {
                T angle;
                T cos_angle;
                while (true)
                {
                        angle = normal_distribution_(engine);
                        if (angle < 0 || angle > PI<T> / 2)
                        {
                                continue;
                        }
                        cos_angle = std::cos(angle);
                        const T f = std::pow(cos_angle, n_) * std::pow(std::sin(angle), p_);
                        const T p = std::exp(normal_distribution_coef_ * square(angle - mean_));
                        if (f > p * urd_(engine))
                        {
                                break;
                        }
                }

                const T n = cos_angle;
                const T length = std::sqrt(1 - square(n));
                const numerical::Vector<N - 1, T> v = length * uniform_on_sphere<N - 1, T>(engine);

                numerical::Vector<N, T> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = v[i];
                }
                res[N - 1] = n;

                return res;
        }

        static PowerCosineOnHemisphere& instance(const T power)
        {
                thread_local std::unordered_map<T, PowerCosineOnHemisphere<N, T>> map;
                const auto iter = map.find(power);
                if (iter != map.end())
                {
                        return iter->second;
                }
                return map.emplace(power, PowerCosineOnHemisphere<N, T>(power)).first->second;
        }
};
}

template <std::size_t N, typename T, typename RandomEngine>
        requires (N > 3)
numerical::Vector<N, T> power_cosine_on_hemisphere(RandomEngine& engine, const std::type_identity_t<T> power)
{
        namespace impl = sphere_power_cosine_implementation;

        return impl::PowerCosineOnHemisphere<N, T>::instance(power).sample(engine);
}

template <std::size_t N, typename T, typename RandomEngine>
        requires (N == 3)
numerical::Vector<N, T> power_cosine_on_hemisphere(RandomEngine& engine, const std::type_identity_t<T> power)
{
        numerical::Vector<N - 1, T> v;
        T v_length_square;

        uniform_in_sphere(engine, v, v_length_square);

        const T n = std::pow(v_length_square, 1 / (1 + power));
        const T new_length_squared = 1 - square(n);
        v *= std::sqrt(new_length_squared / v_length_square);

        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res[i] = v[i];
        }
        res[N - 1] = n;

        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
numerical::Vector<N, T> power_cosine_on_hemisphere(
        RandomEngine& engine,
        const numerical::Vector<N, T>& normal,
        const T power)
{
        const std::array<numerical::Vector<N, T>, N - 1> orthonormal_basis =
                numerical::orthogonal_complement_of_unit_vector(normal);

        const numerical::Vector<N, T> coordinates = power_cosine_on_hemisphere<N, T>(engine, power);

        numerical::Vector<N, T> res = coordinates[N - 1] * normal;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res.multiply_add(coordinates[i], orthonormal_basis[i]);
        }

        return res;
}

template <std::size_t N, typename T>
T power_cosine_on_hemisphere_pdf(const T& n_v, const std::type_identity_t<T>& power)
{
        if (n_v > 0)
        {
                const T k = geometry::shapes::sphere_integrate_power_cosine_factor_over_hemisphere<N, T>(power);
                return std::pow(n_v, power) / k;
        }
        return 0;
}
}
