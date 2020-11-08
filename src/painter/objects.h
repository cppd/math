/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <vector>

namespace painter
{
// Свойства поверхности в точке.
template <size_t N, typename T>
class SurfaceProperties
{
        // Реальный перпендикуляр.
        Vector<N, T> m_geometric_normal;
        // Видимый перпендикуляр к поверхности. Например, при интерполяции
        // перпендикуляра по перпендикулярам в вершинах симплексов.
        std::optional<Vector<N, T>> m_shading_normal;
        // Цвет поверхности.
        Color m_color;
        // Если поверхность является источником света, то цвет этого источника.
        std::optional<Color> m_light_source_color;
        // Коэффициент для диффузного отражения.
        Color::DataType m_diffuse;
        // Коэффициент для прозрачности.
        Color::DataType m_alpha;

public:
        void set_geometric_normal(const Vector<N, T>& normal)
        {
                m_geometric_normal = normal.normalized();
        }
        const Vector<N, T>& geometric_normal() const
        {
                return m_geometric_normal;
        }

        void set_shading_normal(const Vector<N, T>& normal)
        {
                m_shading_normal = normal.normalized();
        }
        const std::optional<Vector<N, T>>& shading_normal() const
        {
                return m_shading_normal;
        }

        void set_color(const Color& color)
        {
                m_color = color;
        }
        const Color& color() const
        {
                return m_color;
        }

        void set_light_source_color(const Color& light_source_color)
        {
                m_light_source_color = light_source_color;
        }
        const std::optional<Color>& light_source_color() const
        {
                return m_light_source_color;
        }

        void set_diffuse(const Color::DataType& diffuse)
        {
                m_diffuse = std::clamp(diffuse, Color::DataType(0), Color::DataType(1));
        }
        const Color::DataType& diffuse() const
        {
                return m_diffuse;
        }

        void set_alpha(const Color::DataType& alpha)
        {
                m_alpha = std::clamp(alpha, Color::DataType(0), Color::DataType(1));
        }
        const Color::DataType& alpha() const
        {
                return m_alpha;
        }
};

// Свойства поверхности надо находить только для ближайшей точки персечения,
// поэтому свойства определяются не сразу, а через этот интерфейс.
template <size_t N, typename T>
class Surface
{
protected:
        virtual ~Surface() = default;

public:
        virtual SurfaceProperties<N, T> properties(const Vector<N, T>& p, const void* intersection_data) const = 0;
};

//

template <size_t N, typename T>
struct LightProperties final
{
        Color color;
        Vector<N, T> direction_to_light;
};

// Источник света, не являющийся видимым объектом.
template <size_t N, typename T>
struct LightSource
{
        virtual ~LightSource() = default;

        virtual LightProperties<N, T> properties(const Vector<N, T>& point) const = 0;
};

//

// Преобразование точки на экране в луч в пространстве.
template <size_t N, typename T>
struct Projector
{
        virtual ~Projector() = default;

        // Размер экрана в пикселях
        virtual const std::array<int, N - 1>& screen_size() const = 0;

        virtual Ray<N, T> ray(const Vector<N - 1, T>& point) const = 0;
};

struct Statistics final
{
        long long pass_count;
        long long pixel_count;
        long long ray_count;
        long long sample_count;
        double previous_pass_duration;
};

// Последовательность пикселов для рисования.
template <size_t N>
struct Paintbrush
{
        virtual ~Paintbrush() = default;

        virtual void first_pass() = 0;

        virtual std::optional<std::array<int_least16_t, N>> next_pixel(
                int previous_pixel_ray_count,
                int previous_pixel_sample_count) = 0;

        virtual bool next_pass() = 0;

        virtual const std::array<int, N>& screen_size() const = 0;
        virtual Statistics statistics() const = 0;
};

template <size_t N, typename T>
struct Intersection final
{
        T distance;
        const Surface<N, T>* surface;
        const void* data;

        // Чтобы не было direct-initializing, например в std::optional
        Intersection() noexcept
        {
        }
};

template <size_t N, typename T>
struct Scene
{
        virtual ~Scene() = default;

        virtual T size() const = 0;

        virtual std::optional<Intersection<N, T>> intersect(const Ray<N, T>& ray) const = 0;
        virtual bool has_intersection(const Ray<N, T>& ray, const T& distance) const = 0;

        virtual const std::vector<const LightSource<N, T>*>& light_sources() const = 0;
        virtual const Projector<N, T>& projector() const = 0;
        virtual const Color& background_color() const = 0;
        virtual const Color& background_light_source_color() const = 0;
};
}
