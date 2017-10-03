/*
Copyright (C) 2017 Topological Manifold

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

#include "ray3.h"
#include "vec2.h"
#include "vec3.h"

// Свойства поверхности в точке.
class SurfaceProperties
{
        // Перпендикуляр к поверхности.
        vec3 m_normal;
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

        void set_normal(const vec3& normal)
        {
                m_normal = normalize(normal);
        }
        const vec3& get_normal() const
        {
                return m_normal;
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
};

// Простой геометрический объект типа треугольника, сферы и т.п.
class GeometricObject
{
protected:
        virtual ~GeometricObject() = default;

public:
        virtual vec3 normal(const vec3& p) const = 0;
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
};

// Источник света, не являющийся видимым объектом.
class LightSource
{
protected:
        virtual ~LightSource() = default;

public:
        virtual void properties(const vec3& point, vec3* color, vec3* vector_from_point_to_light) const = 0;

        LightSource() = default;
        LightSource(const LightSource&) = default;
        LightSource(LightSource&&) = default;
        LightSource& operator=(const LightSource&) = default;
        LightSource& operator=(LightSource&&) = default;
};

// Преобразование точки на экране в луч в трёхмерном пространстве
class Projector
{
protected:
        virtual ~Projector() = default;

public:
        // Ширина экрана в пикселях
        virtual int screen_width() const = 0;
        // Высота экрана в пикселях
        virtual int screen_height() const = 0;

        // Деление пикселя на равные по ширине и высоте части
        // для использования множества лучей на один пиксель
        virtual int pixel_resolution() const = 0;

        // Для точки на экране луч в трёхмерном пространстве
        virtual ray3 ray(const vec2& point) const = 0;

        Projector() = default;
        Projector(const Projector&) = default;
        Projector(Projector&&) = default;
        Projector& operator=(const Projector&) = default;
        Projector& operator=(Projector&&) = default;
};

// Последовательность пикселов для рисования.
struct PixelSequence
{
        virtual ~PixelSequence() = default;
        virtual void get_pixel(int* x, int* y) = 0;
        virtual void release_pixel(int x, int y) = 0;
        virtual int get_pass_count() const = 0;
};
