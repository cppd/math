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

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <optional>
#include <random>
#include <vector>

namespace ns::painter
{
template <typename T>
using RandomEngine = std::conditional_t<std::is_same_v<std::remove_cv<T>, float>, std::mt19937, std::mt19937_64>;

template <std::size_t N, typename T, typename Color>
struct Sample final
{
        Vector<N, T> l;
        T pdf;
        Color brdf;
        bool specular;

        Sample()
        {
        }
};

template <std::size_t N, typename T, typename Color>
class Surface
{
        Vector<N, T> point_;

protected:
        ~Surface() = default;

public:
        explicit Surface(const Vector<N, T>& point) : point_(point)
        {
        }

        const Vector<N, T>& point() const
        {
                return point_;
        }

        virtual Vector<N, T> geometric_normal() const = 0;
        virtual std::optional<Vector<N, T>> shading_normal() const = 0;

        virtual std::optional<Color> light_source() const = 0;

        virtual Color brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const = 0;

        virtual Sample<N, T, Color> sample_brdf(
                RandomEngine<T>& random_engine,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const = 0;
};

template <std::size_t N, typename T, typename Color>
struct LightSourceSample final
{
        Vector<N, T> l;
        T pdf;
        Color radiance;
        std::optional<T> distance;

        LightSourceSample()
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct LightSource
{
        virtual ~LightSource() = default;

        virtual LightSourceSample<N, T, Color> sample(RandomEngine<T>& random_engine, const Vector<N, T>& point)
                const = 0;

        virtual T pdf(const Vector<N, T>& point, const Vector<N, T>& l) const = 0;

        virtual bool is_delta() const = 0;
};

template <std::size_t N, typename T>
struct Projector
{
        virtual ~Projector() = default;

        virtual const std::array<int, N - 1>& screen_size() const = 0;

        virtual Ray<N, T> ray(const Vector<N - 1, T>& point) const = 0;
};

template <std::size_t N, typename T, typename Color>
struct Scene
{
        virtual ~Scene() = default;

        virtual const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const = 0;

        virtual const std::vector<const LightSource<N, T, Color>*>& light_sources() const = 0;

        virtual const Color& background_light() const = 0;

        virtual const Projector<N, T>& projector() const = 0;

        virtual long long thread_ray_count() const noexcept = 0;
};
}
