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

#include "filter.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class PixelFilter final
{
        //radius=1.5;
        //width=radius/2.5;
        //alpha=1/(2*width*width);
        //gaussian[x_]:=Exp[-alpha*x*x];
        //gaussianFilter[x_]:=gaussian[x]-gaussian[radius];
        //max=gaussianFilter[0];
        //triangle[x_]:=If[x<0,max/radius*x+max,-max/radius*x+max];
        //Plot[{gaussianFilter[x],triangle[x]},{x,-radius,radius},PlotRange->Full]
        static constexpr T FILTER_RADIUS = 1.5;
        static constexpr T GAUSSIAN_FILTER_WIDTH = FILTER_RADIUS / 2.5;

        const GaussianFilter<T> filter_{GAUSSIAN_FILTER_WIDTH, FILTER_RADIUS};

public:
        static T radius()
        {
                return FILTER_RADIUS;
        }

        static T contribution(const Color& sample)
        {
                return sample.luminance();
        }

        struct ColorSamples final
        {
                Color sum_color{0};
                T sum_weight{0};
                Color min_color;
                T min_contribution;
                T min_weight;
                Color max_color;
                T max_contribution;
                T max_weight;

                ColorSamples()
                {
                }
        };

        std::optional<ColorSamples> color_samples(
                const Vector<N, T>& center,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors) const
        {
                thread_local std::vector<Color> samples;
                thread_local std::vector<T> contributions;
                thread_local std::vector<T> weights;

                samples.clear();
                contributions.clear();
                weights.clear();

                T min = Limits<T>::max();
                T max = Limits<T>::lowest();
                std::size_t min_i = Limits<std::size_t>::max();
                std::size_t max_i = Limits<std::size_t>::max();

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (!colors[i])
                        {
                                continue;
                        }

                        const T weight = filter_.compute(center - points[i]);
                        ASSERT(weight >= 0);

                        if (!(weight > 0))
                        {
                                continue;
                        }

                        samples.push_back(weight * (*colors[i]));
                        contributions.push_back(contribution(samples.back()));
                        weights.push_back(weight);

                        if (contributions.back() < min)
                        {
                                min = contributions.back();
                                min_i = samples.size() - 1;
                        }

                        if (contributions.back() > max)
                        {
                                max = contributions.back();
                                max_i = samples.size() - 1;
                        }
                }

                if (samples.empty())
                {
                        return std::nullopt;
                }

                ASSERT(min_i < samples.size());
                ASSERT(max_i < samples.size());

                std::optional<ColorSamples> r(std::in_place);

                r->min_color = samples[min_i];
                r->min_contribution = contributions[min_i];
                r->min_weight = weights[min_i];

                r->max_color = samples[max_i];
                r->max_contribution = contributions[max_i];
                r->max_weight = weights[max_i];

                if (samples.size() > 2)
                {
                        for (std::size_t i = 0; i < samples.size(); ++i)
                        {
                                if (i != min_i && i != max_i)
                                {
                                        r->sum_color += samples[i];
                                        r->sum_weight += weights[i];
                                }
                        }
                }

                return r;
        }

        struct BackgroundSamples final
        {
                T sum{0};
                T min;
                T max;

                BackgroundSamples()
                {
                }
        };

        std::optional<BackgroundSamples> background_samples(
                const Vector<N, T>& center,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors) const
        {
                thread_local std::vector<T> weights;

                weights.clear();

                T min = Limits<T>::max();
                T max = Limits<T>::lowest();
                std::size_t min_i = Limits<std::size_t>::max();
                std::size_t max_i = Limits<std::size_t>::max();

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (colors[i])
                        {
                                continue;
                        }

                        const T weight = filter_.compute(center - points[i]);
                        ASSERT(weight >= 0);

                        if (!(weight > 0))
                        {
                                continue;
                        }

                        weights.push_back(weight);

                        if (weight < min)
                        {
                                min = weight;
                                min_i = weights.size() - 1;
                        }

                        if (weight > max)
                        {
                                max = weight;
                                max_i = weights.size() - 1;
                        }
                }

                if (weights.empty())
                {
                        return std::nullopt;
                }

                ASSERT(min_i < weights.size());
                ASSERT(max_i < weights.size());

                std::optional<BackgroundSamples> r(std::in_place);

                r->min = weights[min_i];
                r->max = weights[max_i];

                if (weights.size() > 2)
                {
                        for (std::size_t i = 0; i < weights.size(); ++i)
                        {
                                if (i != min_i && i != max_i)
                                {
                                        r->sum += weights[i];
                                }
                        }
                }

                return r;
        }
};
}
