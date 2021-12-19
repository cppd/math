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
#include <src/numerical/vector.h>

#include <array>
#include <optional>
#include <random>
#include <tuple>
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
protected:
        ~Surface() = default;

public:
        virtual Vector<N, T> point(const Ray<N, T>& ray, const T& distance) const = 0;

        virtual Vector<N, T> geometric_normal(const Vector<N, T>& point) const = 0;
        virtual std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& point) const = 0;

        virtual std::optional<Color> light_source() const = 0;

        virtual Color brdf(
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const = 0;

        virtual T pdf(const Vector<N, T>& point, const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l)
                const = 0;

        virtual Sample<N, T, Color> sample_brdf(
                RandomEngine<T>& random_engine,
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const = 0;
};

template <std::size_t N, typename T, typename Color>
class SurfacePoint final
{
        const Surface<N, T, Color>* surface_;
        Vector<N, T> point_;

public:
        SurfacePoint() : surface_(nullptr)
        {
        }

        SurfacePoint(const Surface<N, T, Color>* const surface, const Ray<N, T>& ray, const T& distance)
                : surface_(surface), point_(surface->point(ray, distance))
        {
        }

        operator bool() const
        {
                return surface_ != nullptr;
        }

        const Vector<N, T>& point() const
        {
                return point_;
        }

        decltype(auto) geometric_normal() const
        {
                return surface_->geometric_normal(point_);
        }

        decltype(auto) shading_normal() const
        {
                return surface_->shading_normal(point_);
        }

        decltype(auto) light_source() const
        {
                return surface_->light_source();
        }

        decltype(auto) brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                return surface_->brdf(point_, n, v, l);
        }

        decltype(auto) pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                return surface_->pdf(point_, n, v, l);
        }

        decltype(auto) sample_brdf(RandomEngine<T>& engine, const Vector<N, T>& n, const Vector<N, T>& v) const
        {
                return surface_->sample_brdf(engine, point_, n, v);
        }
};

template <typename T, typename Color>
struct LightSourceInfo final
{
        T pdf;
        Color radiance;
        std::optional<T> distance;

        LightSourceInfo()
        {
        }
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

        virtual LightSourceInfo<T, Color> info(const Vector<N, T>& point, const Vector<N, T>& l) const = 0;

        virtual LightSourceSample<N, T, Color> sample(RandomEngine<T>& random_engine, const Vector<N, T>& point)
                const = 0;

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

        virtual SurfacePoint<N, T, Color> intersect(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray) const = 0;

        virtual const std::vector<const LightSource<N, T, Color>*>& light_sources() const = 0;

        virtual const Color& background_light() const = 0;

        virtual const Projector<N, T>& projector() const = 0;

        virtual long long thread_ray_count() const noexcept = 0;
};
}
