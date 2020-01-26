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

#include "com/error.h"
#include "com/vec.h"
#include "geometry/core/delaunay.h"
#include "geometry/core/linear_algebra.h"
#include "geometry/core/ridge.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace prune_facets_implementation
{
template <size_t N>
using RidgeData = RidgeDataN<DelaunayFacet<N>>;
template <size_t N>
using RidgeMap = std::unordered_map<Ridge<N>, RidgeData<N>>;
template <size_t N>
using RidgeSet = std::unordered_set<Ridge<N>>;

template <size_t N>
bool boundary_ridge(const std::vector<bool>& interior_vertices, const Ridge<N>& ridge)
{
        for (int v : ridge.vertices())
        {
                if (!interior_vertices[v])
                {
                        return true;
                }
        }
        return false;
}

template <size_t N>
bool sharp_ridge(const std::vector<vec<N>>& points, const std::vector<bool>& interior_vertices, const Ridge<N>& ridge,
                 const RidgeData<N>& ridge_data)
{
        ASSERT(!ridge_data.empty());

        if (boundary_ridge(interior_vertices, ridge))
        {
                return false;
        }

        if (ridge_data.size() == 1)
        {
                // Грань с одним объектом считается острой
                return true;
        }

        // Ортонормированный базис размерности 2 в ортогональном дополнении ребра ridge
        vec<N> e0;
        vec<N> e1;
        ortho_e0_e1(points, ridge.vertices(), ridge_data.cbegin()->point(), &e0, &e1);

        // Координаты вектора первой грани при проецировании в пространство базиса e0, e1.
        vec<N> base_vec = points[ridge_data.cbegin()->point()] - points[ridge.vertices()[0]];
        vec<2> base = vec<2>(dot(e0, base_vec), dot(e1, base_vec)).normalized();
        ASSERT(is_finite(base));

        double cos_plus = 1;
        double cos_minus = 1;
        double sin_plus = 0;
        double sin_minus = 0;

        // Проецирование граней в пространство базиса e0, e1 и вычисление максимальных углов отклонений
        // граней от первой грани по обе стороны.
        for (auto ridge_facet = std::next(ridge_data.cbegin()); ridge_facet != ridge_data.cend(); ++ridge_facet)
        {
                vec<N> facet_vec = points[ridge_facet->point()] - points[ridge.vertices()[0]];
                vec<2> v = vec<2>(dot(e0, facet_vec), dot(e1, facet_vec)).normalized();
                ASSERT(is_finite(v));

                double sine = cross(base, v);
                double cosine = dot(base, v);

                if (sine >= 0)
                {
                        if (cosine < cos_plus)
                        {
                                cos_plus = cosine;
                                sin_plus = sine;
                        }
                }
                else
                {
                        if (cosine < cos_minus)
                        {
                                cos_minus = cosine;
                                sin_minus = sine;
                        }
                }
        }

        // Нужны сравнения с углом 90 градусов, поэтому далее можно обойтись без арккосинусов,
        // используя вместо них знак косинуса.

        // Если любой из двух углов больше или равен 90 градусов, то грань не острая
        if (cos_plus <= 0 || cos_minus <= 0)
        {
                return false;
        }

        // Сумма двух углов, меньших 90, меньше 180 градусов, поэтому для определения суммы углов
        // можно применить формулу косинуса суммы углов
        // cos(a + b) = cos(a)cos(b) - sin(a)sin(b)
        // Здесь нужен модуль синуса, так как ранее в программе sin_minus <= 0.
        double cos_a_plus_b = cos_plus * cos_minus - std::abs(sin_plus * sin_minus);

        // Если сумма углов меньше 90 градусов, то грань острая
        return cos_a_plus_b > 0;
}
}

// Удаление граней, относящихся к острым рёбрам.
// Ребро считается острым, если угол между его двумя последовательными гранями больше 3 * PI / 2
// или, что тоже самое, все грани находятся внутри угла PI / 2. Ребро с одной гранью считается
// острым. Образующиеся после удаления грани новые острые рёбра тоже должны обрабатываться.
template <size_t N>
void prune_facets_incident_to_sharp_ridges(const std::vector<vec<N>>& points,
                                           const std::vector<DelaunayFacet<N>>& delaunay_facets,
                                           const std::vector<bool>& interior_vertices, std::vector<bool>* cocone_facets)
{
        namespace impl = prune_facets_implementation;

        ASSERT(!delaunay_facets.empty() && delaunay_facets.size() == cocone_facets->size());
        ASSERT(points.size() == interior_vertices.size());

        impl::RidgeMap<N> ridge_map;
        std::unordered_map<const DelaunayFacet<N>*, int> facets_ptr;
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if ((*cocone_facets)[i])
                {
                        add_to_ridges(delaunay_facets[i], &ridge_map);
                        facets_ptr.emplace(&delaunay_facets[i], i);
                }
        }

        impl::RidgeSet<N> suspicious_ridges(ridge_map.size());
        for (const auto& e : ridge_map)
        {
                suspicious_ridges.insert(e.first);
        }

        while (!suspicious_ridges.empty())
        {
                impl::RidgeSet<N> tmp_ridges;

                for (const Ridge<N>& r : suspicious_ridges)
                {
                        auto ridge_iter = ridge_map.find(r);
                        if (ridge_iter == ridge_map.cend())
                        {
                                continue;
                        }

                        if (!impl::sharp_ridge(points, interior_vertices, ridge_iter->first, ridge_iter->second))
                        {
                                continue;
                        }

                        // Поместить в отдельную переменную, чтобы не проходить по граням ребра при
                        // одновременнном удалении этих граней
                        std::vector<const DelaunayFacet<N>*> facets_to_remove;
                        facets_to_remove.reserve(ridge_iter->second.size());

                        for (auto d = ridge_iter->second.cbegin(); d != ridge_iter->second.cend(); ++d)
                        {
                                add_to_ridges(*(d->facet()), d->point(), &tmp_ridges);
                                facets_to_remove.push_back(d->facet());

                                // Пометить грань как удалённую
                                auto del = facets_ptr.find(d->facet());
                                ASSERT(del != facets_ptr.cend());
                                (*cocone_facets)[del->second] = false;
                        }

                        for (unsigned f = 0; f < facets_to_remove.size(); ++f)
                        {
                                remove_from_ridges(*(facets_to_remove[f]), &ridge_map);
                        }
                }

                suspicious_ridges = std::move(tmp_ridges);
        }
}
