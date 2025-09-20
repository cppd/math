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

#include <src/com/random/pcg.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class LightSource;

template <std::size_t N, typename T, typename Color>
struct SurfaceSample final
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<N, T> l;
        T pdf;
        Color brdf;

        [[nodiscard]] bool usable() const
        {
                return pdf > 0 && !brdf.is_black();
        }
};

template <std::size_t N, typename T, typename Color>
class Surface
{
        static_assert(std::is_floating_point_v<T>);

protected:
        ~Surface() = default;

public:
        [[nodiscard]] virtual numerical::Vector<N, T> point(const numerical::Ray<N, T>& ray, T distance) const = 0;

        [[nodiscard]] virtual numerical::Vector<N, T> geometric_normal(const numerical::Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual std::optional<numerical::Vector<N, T>> shading_normal(
                const numerical::Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual const LightSource<N, T, Color>* light_source() const = 0;

        [[nodiscard]] virtual Color brdf(
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual T pdf(
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual SurfaceSample<N, T, Color> sample(
                PCG& engine,
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v) const = 0;

        [[nodiscard]] virtual bool is_specular(const numerical::Vector<N, T>& point) const = 0;

        [[nodiscard]] virtual T alpha(const numerical::Vector<N, T>& point) const = 0;
};

template <std::size_t N, typename T, typename Color>
class SurfaceIntersection final
{
        static_assert(std::is_floating_point_v<T>);

        const Surface<N, T, Color>* surface_ = nullptr;
        numerical::Vector<N, T> point_;
        T distance_;

public:
        SurfaceIntersection()
        {
        }

        SurfaceIntersection(
                const Surface<N, T, Color>* const surface,
                const numerical::Ray<N, T>& ray,
                const T distance)
                : surface_(surface),
                  point_(surface->point(ray, distance)),
                  distance_(distance)
        {
        }

        [[nodiscard]] explicit operator bool() const
        {
                return surface_ != nullptr;
        }

        [[nodiscard]] const numerical::Vector<N, T>& point() const
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

        [[nodiscard]] decltype(auto) brdf(
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const
        {
                return surface_->brdf(point_, n, v, l);
        }

        [[nodiscard]] decltype(auto) pdf(
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const
        {
                return surface_->pdf(point_, n, v, l);
        }

        [[nodiscard]] decltype(auto) sample(
                PCG& engine,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v) const
        {
                return surface_->sample(engine, point_, n, v);
        }

        [[nodiscard]] decltype(auto) is_specular() const
        {
                return surface_->is_specular(point_);
        }

        [[nodiscard]] decltype(auto) alpha() const
        {
                return surface_->alpha(point_);
        }
};

template <typename T, typename Color>
struct LightSourceArriveInfo final
{
        static_assert(std::is_floating_point_v<T>);

        T pdf;
        Color radiance;
        std::optional<T> distance;

        [[nodiscard]] bool usable() const
        {
                return pdf > 0 && !radiance.is_black();
        }

        [[nodiscard]] static LightSourceArriveInfo non_usable()
        {
                LightSourceArriveInfo res;
                res.pdf = 0;
                return res;
        }
};

template <std::size_t N, typename T, typename Color>
struct LightSourceArriveSample final
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<N, T> l;
        T pdf;
        Color radiance;
        std::optional<T> distance;

        [[nodiscard]] bool usable() const
        {
                return pdf > 0 && !radiance.is_black();
        }

        [[nodiscard]] static LightSourceArriveSample non_usable()
        {
                LightSourceArriveSample res;
                res.pdf = 0;
                return res;
        }
};

template <std::size_t N, typename T, typename Color>
struct LightSourceLeaveSample final
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Ray<N, T> ray;
        std::optional<numerical::Vector<N, T>> n;
        T pdf_pos;
        T pdf_dir;
        Color radiance;
        bool infinite_distance;
};

template <std::size_t N, typename T, typename Color>
class LightSource
{
        static_assert(std::is_floating_point_v<T>);

public:
        virtual ~LightSource() = default;

        virtual void init(const numerical::Vector<N, T>& scene_center, T scene_radius) = 0;

        [[nodiscard]] virtual LightSourceArriveSample<N, T, Color> arrive_sample(
                PCG& engine,
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n) const = 0;

        [[nodiscard]] virtual LightSourceArriveInfo<T, Color> arrive_info(
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual LightSourceLeaveSample<N, T, Color> leave_sample(PCG& engine) const = 0;

        [[nodiscard]] virtual T leave_pdf_pos(const numerical::Vector<N, T>& dir) const = 0;
        [[nodiscard]] virtual T leave_pdf_dir(const numerical::Vector<N, T>& dir) const = 0;

        [[nodiscard]] virtual std::optional<Color> leave_radiance(const numerical::Vector<N, T>& dir) const = 0;

        [[nodiscard]] virtual Color power() const = 0;

        [[nodiscard]] virtual bool is_delta() const = 0;

        [[nodiscard]] virtual bool is_infinite_area() const = 0;
};

template <std::size_t N, typename T>
class Projector
{
        static_assert(std::is_floating_point_v<T>);

public:
        virtual ~Projector() = default;

        [[nodiscard]] virtual const std::array<int, N - 1>& screen_size() const = 0;

        [[nodiscard]] virtual numerical::Ray<N, T> ray(const numerical::Vector<N - 1, T>& point) const = 0;
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
class Shape
{
        static_assert(std::is_floating_point_v<T>);

public:
        virtual ~Shape() = default;

        [[nodiscard]] virtual T intersection_cost() const = 0;

        [[nodiscard]] virtual std::optional<T> intersect_bounds(const numerical::Ray<N, T>& ray, T max_distance)
                const = 0;

        [[nodiscard]] virtual ShapeIntersection<N, T, Color> intersect(
                const numerical::Ray<N, T>& ray,
                T max_distance,
                T bounding_distance) const = 0;

        [[nodiscard]] virtual bool intersect_any(const numerical::Ray<N, T>& ray, T max_distance, T bounding_distance)
                const = 0;

        [[nodiscard]] virtual geometry::spatial::BoundingBox<N, T> bounding_box() const = 0;

        [[nodiscard]] virtual std::function<
                bool(const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>&)>
                overlap_function() const = 0;
};

template <std::size_t N, typename T, typename Color>
class Scene
{
        static_assert(std::is_floating_point_v<T>);

public:
        virtual ~Scene() = default;

        [[nodiscard]] virtual SurfaceIntersection<N, T, Color> intersect(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray) const = 0;

        [[nodiscard]] virtual SurfaceIntersection<N, T, Color> intersect(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray,
                T max_distance) const = 0;

        [[nodiscard]] virtual bool intersect_any(
                const std::optional<numerical::Vector<N, T>>& geometric_normal,
                const numerical::Ray<N, T>& ray,
                T max_distance) const = 0;

        [[nodiscard]] virtual const std::vector<const LightSource<N, T, Color>*>& light_sources() const = 0;

        [[nodiscard]] virtual const Color& background_color() const = 0;

        [[nodiscard]] virtual const Projector<N, T>& projector() const = 0;

        [[nodiscard]] virtual long long thread_ray_count() const noexcept = 0;
};
}
