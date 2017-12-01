/*
Copyright (C) 2017 Topological Manifold

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

#include "path_tracing/objects.h"

class StratifiedJitteredSampler final : public Sampler
{
        const int m_samples_one_dimension;

public:
        StratifiedJitteredSampler(int samples_per_pixel);

        void generate(std::mt19937_64& random_engine, std::vector<vec2>* samples) const override;
};

class LatinHypercubeSampler final : public Sampler
{
        const int m_samples_per_pixel;

public:
        LatinHypercubeSampler(int samples_per_pixel);

        void generate(std::mt19937_64& random_engine, std::vector<vec2>* samples) const override;
};
