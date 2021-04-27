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

/*
 Matt Pharr, Wenzel Jakob, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Third edition.
 Elsevier, 2017.
 13.3 Sampling random variables
 13.6 2D Sampling with multidimensional transformations.
*/

#pragma once

#include "sphere_uniform.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/math.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <random>
#include <unordered_map>

namespace ns::sampling
{
template <typename RandomEngine, std::size_t N, typename T>
Vector<N, T> cosine_on_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        static_assert(N > 2);

        Vector<N - 1, T> v;
        T v_length_square;

        uniform_in_sphere(random_engine, v, v_length_square);

        T n = std::sqrt(1 - v_length_square);

        std::array<Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> res = n * normal;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                res += v[i] * basis[i];
        }

        return res;
}

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
template <std::size_t N, typename T>
class PowerCosineOnHemisphere
{
        static_assert(N > 3);

        T m_n;
        T m_p;
        T m_mean;
        T m_normal_distribution_coef;
        std::normal_distribution<T> m_normal_distribution;
        std::uniform_real_distribution<T> m_urd;

public:
        explicit PowerCosineOnHemisphere(std::type_identity_t<T> power)
        {
                if (!(power >= 1))
                {
                        error("Power for cosine " + to_string(power) + " must be greater than or equal to 1");
                }

                m_n = power;
                m_p = N - 2;
                m_mean = std::atan(std::sqrt(m_p / m_n));
                T deviation = T(1) / std::sqrt((m_n + m_p) * std::sqrt(T(2)));
                m_normal_distribution = std::normal_distribution<T>(m_mean, deviation);
                m_normal_distribution_coef = T(-1) / (T(2) * square(deviation));
                T max = std::pow(std::cos(m_mean), m_n) * std::pow(std::sin(m_mean), m_p);
                m_urd = std::uniform_real_distribution<T>(0, max);
        }

        template <typename RandomEngine>
        Vector<N, T> sample(RandomEngine& random_engine, const Vector<N, T>& normal)
        {
                T angle;
                T cos_angle;
                while (true)
                {
                        angle = m_normal_distribution(random_engine);
                        if (angle < 0 || angle > PI<T> / 2)
                        {
                                continue;
                        }
                        cos_angle = std::cos(angle);
                        T f = std::pow(cos_angle, m_n) * std::pow(std::sin(angle), m_p);
                        T p = std::exp(m_normal_distribution_coef * square(angle - m_mean));
                        if (f > p * m_urd(random_engine))
                        {
                                break;
                        }
                }

                T n = cos_angle;
                T length = std::sqrt(1 - square(n));
                Vector<N - 1, T> v = length * uniform_on_sphere<N - 1, T>(random_engine);

                std::array<Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

                Vector<N, T> res = n * normal;

                for (unsigned i = 0; i < N - 1; ++i)
                {
                        res += v[i] * basis[i];
                }

                return res;
        }
};

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> power_cosine_on_hemisphere(RandomEngine& random_engine, const Vector<N, T>& normal, T power)
{
        static_assert(N > 3);

        thread_local std::unordered_map<T, PowerCosineOnHemisphere<N, T>> map;
        auto iter = map.find(power);
        if (iter != map.end())
        {
                return iter->second.sample(random_engine, normal);
        }
        return map.emplace(power, PowerCosineOnHemisphere<N, T>(power)).first->second.sample(random_engine, normal);
}

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
template <typename T, typename RandomEngine>
Vector<3, T> power_cosine_on_hemisphere(RandomEngine& random_engine, const Vector<3, T>& normal, T power)
{
        constexpr unsigned N = 3;

        Vector<N - 1, T> v;
        T v_length_square;

        uniform_in_sphere(random_engine, v, v_length_square);

        T n = std::pow(v_length_square, 1 / (1 + power));
        T new_length_squared = 1 - square(n);
        v *= std::sqrt(new_length_squared / v_length_square);

        std::array<Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<N, T> res = n * normal;

        for (unsigned i = 0; i < N - 1; ++i)
        {
                res += v[i] * basis[i];
        }

        return res;
}

template <std::size_t N, typename T>
T cosine_on_hemisphere_pdf(T n_v)
{
        if (n_v > 0)
        {
                static constexpr T k = 1 / geometry::sphere_integrate_cosine_factor_over_hemisphere(N);
                return n_v * k;
        }
        return 0;
}

template <std::size_t N, typename T>
T power_cosine_on_hemisphere_pdf(T n_v, std::type_identity_t<T> power)
{
        if (n_v > 0)
        {
                const T k = geometry::sphere_integrate_power_cosine_factor_over_hemisphere<N, T>(power);
                return std::pow(n_v, power) / k;
        }
        return 0;
}
}
