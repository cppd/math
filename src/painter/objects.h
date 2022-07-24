/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/random/pcg.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
struct SurfaceSample final
{
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> l;
        T pdf;
        Color brdf;
        bool specular;

        SurfaceSample()
        {
        }
};

template <std::size_t N, typename T, typename Color>
class Surface
{
        static_assert(std::is_floating_point_v<T>);

protected:
        ~Surface() = default;

public:
        [[nodiscard]] virtual Vector<N, T> point(const Ray<N, T>& ray, T distance) const = 0;

        [[nodiscard]] virtual Vector<N, T> geometric_normal(const Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual std::optional<Color> light_source() const = 0;

        [[nodiscard]] virtual Color brdf(
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual T pdf(
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual SurfaceSample<N, T, Color> sample(
                PCG& engine,
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const = 0;
};

template <std::size_t N, typename T, typename Color>
class SurfaceIntersection final
{
        static_assert(std::is_floating_point_v<T>);

        const Surface<N, T, Color>* surface_ = nullptr;
        Vector<N, T> point_;
        T distance_;

public:
        SurfaceIntersection()
        {
        }

        SurfaceIntersection(const Surface<N, T, Color>* const surface, const Ray<N, T>& ray, const T distance)
                : surface_(surface),
                  point_(surface->point(ray, distance)),
                  distance_(distance)
        {
        }

        [[nodiscard]] explicit operator bool() const
        {
                return surface_ != nullptr;
        }

        [[nodiscard]] const Vector<N, T>& point() const
        {
                return point_;
        }

        [[nodiscard]] T distance() const
        {
                return distance_;
        }

        [[nodiscard]] decltype(auto) geometric_normal() const
        {
                return surface_->geometric_normal(point_);
        }

        [[nodiscard]] decltype(auto) shading_normal() const
        {
                return surface_->shading_normal(point_);
        }

        [[nodiscard]] decltype(auto) light_source() const
        {
                return surface_->light_source();
        }

        [[nodiscard]] decltype(auto) brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                return surface_->brdf(point_, n, v, l);
        }

        [[nodiscard]] decltype(auto) pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                return surface_->pdf(point_, n, v, l);
        }

        [[nodiscard]] decltype(auto) sample(PCG& engine, const Vector<N, T>& n, const Vector<N, T>& v) const
        {
                return surface_->sample(engine, point_, n, v);
        }
};

template <typename T, typename Color>
struct LightSourceInfo final
{
        static_assert(std::is_floating_point_v<T>);

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
        static_assert(std::is_floating_point_v<T>);

        Vector<N, T> l;
        T pdf;
        Color radiance;
        std::optional<T> distance;

        LightSourceSample()
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct LightSourceSampleEmit final
{
        static_assert(std::is_floating_point_v<T>);

        Ray<N, T> ray;
        Vector<N, T> n;
        T pdf_pos;
        T pdf_dir;
        Color radiance;

        LightSourceSampleEmit()
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct LightSource
{
        static_assert(std::is_floating_point_v<T>);

        virtual ~LightSource() = default;

        [[nodiscard]] virtual LightSourceSample<N, T, Color> sample(PCG& engine, const Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual LightSourceInfo<T, Color> info(const Vector<N, T>& point, const Vector<N, T>& l)
                const = 0;

        [[nodiscard]] virtual LightSourceSampleEmit<N, T, Color> sample_emit(PCG& engine) const = 0;

        [[nodiscard]] virtual bool is_delta() const = 0;
};

template <std::size_t N, typename T>
struct Projector
{
        static_assert(std::is_floating_point_v<T>);

        virtual ~Projector() = default;

        [[nodiscard]] virtual const std::array<int, N - 1>& screen_size() const = 0;

        [[nodiscard]] virtual Ray<N, T> ray(const Vector<N - 1, T>& point) const = 0;
};

template <std::size_t N, typename T, typename Color>
struct ShapeIntersection final
{
        static_assert(std::is_floating_point_v<T>);

        T distance;
        const Surface<N, T, Color>* surface;

        ShapeIntersection(const T distance, const Surface<N, T, Color>* const surface)
                : distance(distance),
                  surface(surface)
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct Shape
{
        static_assert(std::is_floating_point_v<T>);

        virtual ~Shape() = default;

        [[nodiscard]] virtual T intersection_cost() const = 0;

        [[nodiscard]] virtual std::optional<T> intersect_bounds(const Ray<N, T>& ray, T max_distance) const = 0;

        [[nodiscard]] virtual ShapeIntersection<N, T, Color> intersect(
                const Ray<N, T>& ray,
                T max_distance,
                T bounding_distance) const = 0;

        [[nodiscard]] virtual bool intersect_any(const Ray<N, T>& ray, T max_distance, T bounding_distance) const = 0;

        [[nodiscard]] virtual geometry::BoundingBox<N, T> bounding_box() const = 0;

        [[nodiscard]] virtual std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)>
                overlap_function() const = 0;
};

template <std::size_t N, typename T, typename Color>
struct Scene
{
        static_assert(std::is_floating_point_v<T>);

        virtual ~Scene() = default;

        [[nodiscard]] virtual SurfaceIntersection<N, T, Color> intersect(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray) const = 0;

        [[nodiscard]] virtual SurfaceIntersection<N, T, Color> intersect(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray,
                T max_distance) const = 0;

        [[nodiscard]] virtual bool intersect_any(
                const std::optional<Vector<N, T>>& geometric_normal,
                const Ray<N, T>& ray,
                T max_distance) const = 0;

        [[nodiscard]] virtual const std::vector<const LightSource<N, T, Color>*>& light_sources() const = 0;

        [[nodiscard]] virtual const Color& background_light() const = 0;

        [[nodiscard]] virtual const Projector<N, T>& projector() const = 0;

        [[nodiscard]] virtual long long thread_ray_count() const noexcept = 0;
};
}
