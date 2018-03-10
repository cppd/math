/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/color/colors.h"
#include "com/ray.h"
#include "com/vec.h"

#include <vector>

// Свойства поверхности в точке.
class SurfaceProperties
{
        // Реальный перпендикуляр.
        vec3 m_geometric_normal;
        // Видимый перпендикуляр к поверхности. Например, при интерполяции
        // перпендикуляра по перпендикулярам в вершинах треугольников.
        vec3 m_shading_normal;
        // Цвет поверхности.
        Color m_color;
        // Если поверхность является источником света, то цвет этого источника.
        Color m_light_source_color;
        // Коэффициент для диффузного отражения и коэффициент для отражения и преломления по формулам Френеля.
        double m_diffuse;
        double m_fresnel;
        // Коэффициент преломления.
        double m_refraction;
        // Является ли поверхность источником света.
        bool m_light_source;
        // Является ли поверхность набором треугольников
        bool m_triangle_mesh = false;

public:
        SurfaceProperties() = default;

        SurfaceProperties(const Color& color, const Color& light_source_color, double diffuse, bool use_fresnel,
                          double refraction, bool light_source)
                : m_color(color),
                  m_light_source_color(light_source_color),
                  m_diffuse(diffuse),
                  m_fresnel(use_fresnel),
                  m_refraction(refraction),
                  m_light_source(light_source)
        {
        }

        void set_shading_normal(const vec3& normal)
        {
                m_shading_normal = normalize(normal);
        }
        const vec3& get_shading_normal() const
        {
                return m_shading_normal;
        }

        void set_geometric_normal(const vec3& normal)
        {
                m_geometric_normal = normalize(normal);
        }
        const vec3& get_geometric_normal() const
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

        void set_diffuse_and_fresnel(double diffuse, double fresnel)
        {
                m_diffuse = diffuse;
                m_fresnel = fresnel;
        }
        double get_diffuse() const
        {
                return m_diffuse;
        }
        double get_fresnel() const
        {
                return m_fresnel;
        }

        void set_refraction(double refraction)
        {
                m_refraction = refraction;
        }
        double get_refraction() const
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

        void set_triangle_mesh(bool triangle_mesh)
        {
                m_triangle_mesh = triangle_mesh;
        }
        bool is_triangle_mesh() const
        {
                return m_triangle_mesh;
        }
};

// Свойства поверхности надо находить только для ближайшей точки персечения,
// поэтому свойства определяются не сразу, а через этот интерфейс.
class Surface
{
protected:
        virtual ~Surface() = default;

public:
        virtual SurfaceProperties properties(const vec3& p, const void* intersection_data) const = 0;
};

// Один объект или структура из объектов, элементами которой
// могут быть объекты или структуры из объектов и т.д.
struct GenericObject
{
        virtual ~GenericObject() = default;

        // Для случая структуры из объектов это пересечение луча с границей структуры.
        // Для случая одного объекта это пересечение луча с самим объектом.
        virtual bool intersect_approximate(const ray3& r, double* t) const = 0;

        // Для случая структуры из объектов это пересечение луча с объектом.
        // Для случая одного объекта это пересечение луча с самим объектом,
        // уже полученное функцией intersect_approximate.
        virtual bool intersect_precise(const ray3&, double approximate_t, double* t, const Surface** surface,
                                       const void** intersection_data) const = 0;
};

// Источник света, не являющийся видимым объектом.
struct LightSource
{
        virtual ~LightSource() = default;

        virtual void properties(const vec3& point, Color* color, vec3* vector_from_point_to_light) const = 0;
};

// Преобразование точки на экране в луч в трёхмерном пространстве.
struct Projector
{
        virtual ~Projector() = default;

        // Ширина экрана в пикселях
        virtual int screen_width() const = 0;
        // Высота экрана в пикселях
        virtual int screen_height() const = 0;

        // Для точки на экране луч в трёхмерном пространстве
        virtual ray3 ray(const vec2& point) const = 0;
};

// Последовательность пикселов для рисования.
struct Paintbrush
{
        virtual ~Paintbrush() = default;

        virtual void first_pass() noexcept = 0;
        virtual bool next_pixel(int previous_pixel_ray_count, int previous_pixel_sample_count,
                                std::array<int_least16_t, 2>* pixel) noexcept = 0;
        virtual void next_pass() noexcept = 0;

        virtual int painting_width() const noexcept = 0;
        virtual int painting_height() const noexcept = 0;

        virtual void statistics(long long* pass_count, long long* pixel_count, long long* ray_count, long long* sample_count,
                                double* previous_pass_duration) const noexcept = 0;
};

// Объекты для рисования.
struct PaintObjects
{
        virtual ~PaintObjects() = default;

        virtual const std::vector<const GenericObject*>& objects() const = 0;
        virtual const std::vector<const LightSource*>& light_sources() const = 0;

        virtual const Projector& projector() const = 0;

        virtual const SurfaceProperties& default_surface_properties() const = 0;
};
