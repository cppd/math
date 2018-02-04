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

#include "com/ray.h"
#include "com/vec.h"

#include <random>
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
        vec3 m_color;
        // Если поверхность является источником света, то цвет этого источника.
        vec3 m_light_source_color;
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

        SurfaceProperties(const vec3& color, const vec3& light_source_color, double diffuse, bool use_fresnel, double refraction,
                          bool light_source)
                : // без m_normal
                  m_color(color),
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

        void set_color(const vec3& color)
        {
                m_color = color;
        }
        const vec3& get_color() const
        {
                return m_color;
        }

        void set_light_source_color(const vec3& light_source_color)
        {
                m_light_source_color = light_source_color;
        }
        const vec3& get_light_source_color() const
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

// Простой геометрический объект типа треугольника, сферы и т.п.
class GeometricObject
{
protected:
        virtual ~GeometricObject() = default;

public:
        virtual bool intersect(const ray3& r, double* t) const = 0;

        GeometricObject() = default;
        GeometricObject(const GeometricObject&) = default;
        GeometricObject(GeometricObject&&) = default;
        GeometricObject& operator=(const GeometricObject&) = default;
        GeometricObject& operator=(GeometricObject&&) = default;
};

// Свойства поверхности надо находить только для ближайшей точки персечения, поэтому свойства
// определяются не сразу, а через этот интерфейс.
class Surface
{
protected:
        virtual ~Surface() = default;

public:
        virtual SurfaceProperties properties(const vec3& p, const GeometricObject* geometric_object) const = 0;

        Surface() = default;
        Surface(const Surface&) = default;
        Surface(Surface&&) = default;
        Surface& operator=(const Surface&) = default;
        Surface& operator=(Surface&&) = default;
};

// Общий объект вроде геометрического объекта или структуры из объектов (например, октадерева),
// элементами которой могут быть геометрические объекты или другие структуры из объектов.
class GenericObject
{
protected:
        virtual ~GenericObject() = default;

public:
        // Для случая структуры из объектов это пересечение луча с границей структуры.
        // Для случая геометрического объекта это пересечение луча с самим объектом.
        virtual bool intersect_approximate(const ray3& r, double* t) const = 0;

        // Для случая структуры из объектов это пересечение луча с геометрическим объектом.
        // Для случая геометрического объекта это будет пересечение луча с самим объектом,
        // уже полученное функцией intersect_approximate.
        virtual bool intersect_precise(const ray3&, double approximate_t, double* t, const Surface** surface,
                                       const GeometricObject** geometric_object) const = 0;

        GenericObject() = default;
        GenericObject(const GenericObject&) = default;
        GenericObject(GenericObject&&) = default;
        GenericObject& operator=(const GenericObject&) = default;
        GenericObject& operator=(GenericObject&&) = default;
};

// Источник света, не являющийся видимым объектом.
class LightSource
{
public:
        virtual ~LightSource() = default;

        virtual void properties(const vec3& point, vec3* color, vec3* vector_from_point_to_light) const = 0;

        LightSource() = default;
        LightSource(const LightSource&) = default;
        LightSource(LightSource&&) = default;
        LightSource& operator=(const LightSource&) = default;
        LightSource& operator=(LightSource&&) = default;
};

// Преобразование точки на экране в луч в трёхмерном пространстве.
class Projector
{
public:
        virtual ~Projector() = default;

        // Ширина экрана в пикселях
        virtual int screen_width() const = 0;
        // Высота экрана в пикселях
        virtual int screen_height() const = 0;

        // Для точки на экране луч в трёхмерном пространстве
        virtual ray3 ray(const vec2& point) const = 0;

        Projector() = default;
        Projector(const Projector&) = default;
        Projector(Projector&&) = default;
        Projector& operator=(const Projector&) = default;
        Projector& operator=(Projector&&) = default;
};

// Последовательность пикселов для рисования.
class Paintbrush
{
protected:
        virtual ~Paintbrush() = default;

public:
        virtual void get_pixel(int* x, int* y) = 0;
        virtual void release_pixel(int x, int y) = 0;

        virtual void pass_and_pixel_count(int*, long long*) const = 0;

        Paintbrush() = default;
        Paintbrush(const Paintbrush&) = default;
        Paintbrush(Paintbrush&&) = default;
        Paintbrush& operator=(const Paintbrush&) = default;
        Paintbrush& operator=(Paintbrush&&) = default;
};

// Объекты для рисования
class PaintObjects
{
public:
        virtual ~PaintObjects() = default;

        virtual const std::vector<const GenericObject*>& objects() const = 0;
        virtual const std::vector<const LightSource*>& light_sources() const = 0;
        virtual const Projector& projector() const = 0;
        virtual const SurfaceProperties& default_surface_properties() const = 0;
};
