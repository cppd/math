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

#include <src/color/color.h>
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

template <std::size_t N, typename T>
struct SurfaceProperties final
{
        // Реальный перпендикуляр.
        Vector<N, T> geometric_normal;
        // Видимый перпендикуляр к поверхности. Например, при интерполяции
        // перпендикуляра по перпендикулярам в вершинах симплексов.
        std::optional<Vector<N, T>> shading_normal;
        // Если поверхность является источником света, то цвет этого источника.
        std::optional<Color> light_source_color;

        SurfaceProperties()
        {
        }
};

template <std::size_t N, typename T>
struct SurfaceSample final
{
        Vector<N, T> l;
        Color color;

        SurfaceSample()
        {
        }
};

enum class SurfaceSampleType
{
        Reflection,
        Transmission
};

// Свойства поверхности надо находить только для ближайшей точки персечения,
// поэтому свойства определяются не сразу, а через этот интерфейс.
template <std::size_t N, typename T>
class Surface
{
protected:
        virtual ~Surface() = default;

public:
        virtual SurfaceProperties<N, T> properties(const Vector<N, T>& point, const void* intersection_data) const = 0;

        virtual Color shade(
                const Vector<N, T>& point,
                const void* intersection_data,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const = 0;

        virtual SurfaceSample<N, T> sample_shade(
                RandomEngine<T>& random_engine,
                SurfaceSampleType sample_type,
                const Vector<N, T>& point,
                const void* intersection_data,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const = 0;
};

template <std::size_t N, typename T>
struct Intersection final
{
        const Surface<N, T>* surface;
        Vector<N, T> point;
        const void* data;

        // Чтобы не было direct-initializing, например в std::optional
        Intersection() noexcept
        {
        }
};

template <std::size_t N, typename T>
struct LightSourceSample final
{
        Vector<N, T> l;
        Color::DataType pdf;
        Color color;
        std::optional<T> distance;

        LightSourceSample()
        {
        }
};

// Источник света, не являющийся видимым объектом.
template <std::size_t N, typename T>
struct LightSource
{
        virtual ~LightSource() = default;

        virtual LightSourceSample<N, T> sample(const Vector<N, T>& point) const = 0;
};

// Преобразование точки на экране в луч в пространстве.
template <std::size_t N, typename T>
struct Projector
{
        virtual ~Projector() = default;

        // Размер экрана в пикселях
        virtual const std::array<int, N - 1>& screen_size() const = 0;

        virtual Ray<N, T> ray(const Vector<N - 1, T>& point) const = 0;
};

template <std::size_t N, typename T>
struct Scene
{
        virtual ~Scene() = default;

        virtual std::optional<Intersection<N, T>> intersect(const Ray<N, T>& ray) const = 0;

        virtual const std::vector<const LightSource<N, T>*>& light_sources() const = 0;
        virtual const Projector<N, T>& projector() const = 0;
        virtual const Color& background_color() const = 0;
        virtual const Color& background_light_source_color() const = 0;

        virtual long long thread_ray_count() const noexcept = 0;
};
}
