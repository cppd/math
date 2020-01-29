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

#include <vector>

// Свойства поверхности в точке.
template <size_t N, typename T>
class SurfaceProperties
{
        // Реальный перпендикуляр.
        Vector<N, T> m_geometric_normal;
        // Видимый перпендикуляр к поверхности. Например, при интерполяции
        // перпендикуляра по перпендикулярам в вершинах симплексов.
        Vector<N, T> m_shading_normal;
        // Цвет поверхности.
        Color m_color;
        // Если поверхность является источником света, то цвет этого источника.
        Color m_light_source_color;
        // Коэффициент для диффузного отражения и коэффициент для отражения и преломления по формулам Френеля.
        T m_diffuse;
        T m_fresnel;
        // Коэффициент преломления.
        T m_refraction;
        // Является ли поверхность источником света.
        bool m_light_source;
        // Является ли поверхность набором симплексов
        bool m_mesh = false;

public:
        SurfaceProperties() = default;

        SurfaceProperties(
                const Color& color,
                const Color& light_source_color,
                T diffuse,
                bool use_fresnel,
                T refraction,
                bool light_source)
                : m_color(color),
                  m_light_source_color(light_source_color),
                  m_diffuse(diffuse),
                  m_fresnel(use_fresnel),
                  m_refraction(refraction),
                  m_light_source(light_source)
        {
        }

        void set_shading_normal(const Vector<N, T>& normal)
        {
                m_shading_normal = normal.normalized();
        }
        const Vector<N, T>& get_shading_normal() const
        {
                return m_shading_normal;
        }

        void set_geometric_normal(const Vector<N, T>& normal)
        {
                m_geometric_normal = normal.normalized();
        }
        const Vector<N, T>& get_geometric_normal() const
        {
                return m_geometric_normal;
        }

        void set_color(const Color& color)
        {
                m_color = color;
        }
        const Color& get_color() const
        {
                return m_color;
        }

        void set_light_source_color(const Color& light_source_color)
        {
                m_light_source_color = light_source_color;
        }
        const Color& get_light_source_color() const
        {
                return m_light_source_color;
        }

        void set_diffuse_and_fresnel(T diffuse, T fresnel)
        {
                m_diffuse = diffuse;
                m_fresnel = fresnel;
        }
        T get_diffuse() const
        {
                return m_diffuse;
        }
        T get_fresnel() const
        {
                return m_fresnel;
        }

        void set_refraction(T refraction)
        {
                m_refraction = refraction;
        }
        T get_refraction() const
        {
                return m_refraction;
        }

        void set_light_source(bool light_source)
        {
                m_light_source = light_source;
        }
        bool is_light_source() const
        {
                return m_light_source;
        }

        void set_mesh(bool mesh)
        {
                m_mesh = mesh;
        }
        bool is_mesh() const
        {
                return m_mesh;
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

// Один объект или структура из объектов, элементами которой
// могут быть объекты или структуры из объектов и т.д.
template <size_t N, typename T>
struct GenericObject
{
        virtual ~GenericObject() = default;

        // Для случая структуры из объектов это пересечение луча с границей структуры.
        // Для случая одного объекта это пересечение луча с самим объектом.
        virtual bool intersect_approximate(const Ray<N, T>& r, T* t) const = 0;

        // Для случая структуры из объектов это пересечение луча с объектом.
        // Для случая одного объекта это пересечение луча с самим объектом,
        // уже полученное функцией intersect_approximate.
        virtual bool intersect_precise(
                const Ray<N, T>&,
                T approximate_t,
                T* t,
                const Surface<N, T>** surface,
                const void** intersection_data) const = 0;

        virtual void min_max(Vector<N, T>* min, Vector<N, T>* max) const = 0;
};

// Источник света, не являющийся видимым объектом.
template <size_t N, typename T>
struct LightSource
{
        virtual ~LightSource() = default;

        virtual void properties(const Vector<N, T>& point, Color* color, Vector<N, T>* vector_from_point_to_light)
                const = 0;
};

// Преобразование точки на экране в луч в пространстве.
template <size_t N, typename T>
struct Projector
{
        virtual ~Projector() = default;

        // Размер экрана в пикселях
        virtual const std::array<int, N - 1>& screen_size() const = 0;

        virtual Ray<N, T> ray(const Vector<N - 1, T>& point) const = 0;
};

// Последовательность пикселов для рисования.
template <size_t N>
struct Paintbrush
{
        virtual ~Paintbrush() = default;

        virtual void first_pass() = 0;
        virtual bool next_pixel(
                int previous_pixel_ray_count,
                int previous_pixel_sample_count,
                std::array<int_least16_t, N>* pixel) = 0;
        virtual bool next_pass() = 0;

        virtual const std::array<int, N>& screen_size() const = 0;

        virtual void statistics(
                long long* pass_count,
                long long* pixel_count,
                long long* ray_count,
                long long* sample_count,
                double* previous_pass_duration) const = 0;
};

// Объекты для рисования.
template <size_t N, typename T>
struct PaintObjects
{
        virtual ~PaintObjects() = default;

        virtual const std::vector<const GenericObject<N, T>*>& objects() const = 0;
        virtual const std::vector<const LightSource<N, T>*>& light_sources() const = 0;

        virtual const Projector<N, T>& projector() const = 0;

        virtual const SurfaceProperties<N, T>& default_surface_properties() const = 0;
};
