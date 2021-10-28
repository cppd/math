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

#include "../halton_sampler.h"
#include "../lh_sampler.h"
#include "../sj_sampler.h"

#include <string_view>

namespace ns::sampling::test
{
template <std::size_t N, typename T>
std::string_view sampler_name(const StratifiedJitteredSampler<N, T>&)
{
        return "Stratified Jittered Sampler";
}

template <std::size_t N, typename T>
std::string_view sampler_name(const LatinHypercubeSampler<N, T>&)
{
        return "Latin Hypercube Sampler";
}

template <std::size_t N, typename T>
std::string_view sampler_name(const HaltonSampler<N, T>&)
{
        return "Halton Sampler";
}

//

template <typename T>
constexpr std::string_view random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::mt19937>
{
        return "std::mt19937";
}

template <typename T>
constexpr std::string_view random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::mt19937_64>
{
        return "std::mt19937_64";
}
}
