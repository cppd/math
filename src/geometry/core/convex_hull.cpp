/*
Copyright (C) 2017-2019 Topological Manifold

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

/*
В частности, по книгам

Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

Mark de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars.
Computational Geometry. Algorithms and Applications. Third Edition.
Springer-Verlag Berlin Heidelberg, 2008.

Handbook of Discrete and Computational Geometry.
Edited by Jacob E. Goodman, Joseph O’Rourke.
Chapman & Hall/CRC, 2004.

  Инкрементальный алгоритм построения выпуклой оболочки с применением списков конфликтов,
как это описано в главе 11 книги «Computational Geometry.  Algorithms and Applications».

  Для алгоритма со списками конфликтов важен случайный порядок обработки точек.

  Выпуклая оболочка в n-мерном пространстве состоит из (n-1)-симплексов-граней-facet,
соединённых (n-2)-симплексами-ребрами-ridge. Начальный n-симплекс состоит из n+1 точки,
образующих n линейно независимых n-векторов.
  Каждое ребро соединяет две грани. При добавлении новой точки выбрасываются грани,
видимые из данной точки. Горизонтом видимости являются рёбра, имеющие одну грань.
К каждому из них надо добавить новую точку, формируя тем самым новую грань выпуклой
оболочки.
  Каждая грань имеет список видимых из неё точек. Каждая точка имеет список видимых
из неё граней. Возможные точки для новой грани берутся у двух имеющихся граней
горизонта, одна из которых остаётся, другая удаляется.

  Сиплексы Делоне на основе выпуклой оболочки: исходные точки располагаются на параболиде
размерности n + 1, определяется его выпуклая оболочка, затем выбираются грани с отрицательной
последней координатой (нижняя часть (n+1)-мерного параболоида) и проецируются в исходное
пространство.
*/

#include "convex_hull.h"

#include "array_elements.h"
#include "facet.h"
#include "linear_algebra.h"
#include "max_determinant.h"
#include "ridge.h"

#include "com/combinatorics.h"
#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/names.h"
#include "com/print.h"
#include "com/thread_pool.h"
#include "com/type/integer.h"
#include "com/type/limit.h"
#include "com/type/trait.h"
#include "com/vec.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <random>
#include <unordered_map>
#include <unordered_set>

//   Входные данные переводятся в целые числа с диапазоном от 0 до максимума с заданной дискретизацией,
// чтобы иметь абсолютную точность при вычислении выпуклой оболочки, но не иметь всех проблем с плавающей
// точкой, точность которой надо было бы менять в процессе расчёта.
//   Если используются 24 бита, то максимальные значения скалярных произведений (определители)
// попадают в диапазон __int128 для количества измерений не больше 4 при расчётах на параболоиде
// и не больше 5 при расчётах без параболоида.
//   Для файлов OBJ с типом float вполне достаточно иметь 24 бита.

constexpr int ORDINARY_BITS = 30;
constexpr int PARABOLOID_BITS = 24;

template <size_t N>
using ComputeTypeOrdinary = LeastSignedInteger<max_determinant<N, ORDINARY_BITS>()>;
template <size_t N>
using DataTypeOrdinary = LeastSignedInteger<ORDINARY_BITS>;

template <size_t N>
using ComputeTypeParaboloid = LeastSignedInteger<max_paraboloid_determinant<N, PARABOLOID_BITS>()>;
template <size_t N>
using DataTypeParaboloid = LeastSignedInteger<max_paraboloid_source<N, PARABOLOID_BITS>()>;
template <size_t N>
using ComputeTypeAfterParaboloid = LeastSignedInteger<max_determinant<N, PARABOLOID_BITS>()>;
template <size_t N>
using DataTypeAfterParaboloid = LeastSignedInteger<PARABOLOID_BITS>;

// Не надо медленный mpz_class на основных измерениях
static_assert(is_native_integral<DataTypeOrdinary<2>> && is_native_integral<ComputeTypeOrdinary<2>>);
static_assert(is_native_integral<DataTypeOrdinary<3>> && is_native_integral<ComputeTypeOrdinary<3>>);
static_assert(is_native_integral<DataTypeOrdinary<4>> && is_native_integral<ComputeTypeOrdinary<4>>);
static_assert(is_native_integral<DataTypeParaboloid<3>> && is_native_integral<ComputeTypeParaboloid<3>>);
static_assert(is_native_integral<DataTypeAfterParaboloid<2>> && is_native_integral<ComputeTypeAfterParaboloid<2>>);
static_assert(is_native_integral<DataTypeParaboloid<4>> && is_native_integral<ComputeTypeParaboloid<4>>);
static_assert(is_native_integral<DataTypeAfterParaboloid<3>> && is_native_integral<ComputeTypeAfterParaboloid<3>>);
//

template <typename F>
using FacetList = std::list<F>;
template <typename F>
using FacetListConstIterator = typename FacetList<F>::const_iterator;
template <size_t N, typename DataType, typename ComputeType>
using Facet = FacetInteger<N, DataType, ComputeType, FacetListConstIterator>;

namespace
{
template <typename T>
std::enable_if_t<is_native_integral<T>, std::string> type_str()
{
        return to_string(limits<T>::digits) + " bits";
}
template <typename T>
std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, mpz_class>, std::string> type_str()
{
        return "mpz_class";
}

// Количество потоков для обработки горизонта.
// В одних случаях параллельность ускоряет (точки внутри сферы), в других замедляет (точки на поверхности сферы).
// При использовании mpz_class параллельность ускоряет.
template <typename S, typename C>
int thread_count() noexcept
{
        static_assert(is_integral<S> && is_integral<C>);

        if (is_native_integral<S> && is_native_integral<C>)
        {
                return 1;
        }
        else
        {
                int hc = hardware_concurrency();
                return std::max(hc - 1, 1);
        }
}

template <typename T>
class FacetStore
{
        // здесь vector быстрее, чем forward_list, list, set, unordered_set
        std::vector<const T*> m_data;

public:
        void insert(const T* f)
        {
                m_data.push_back(f);
        }
#if 0
        void erase(const T* f)
        {
                m_data.erase(std::find(m_data.cbegin(), m_data.cend(), f));
        }
#endif
#if 0
        void erase(const T* f)
        {
                int_fast32_t size = m_data.size();
                int_fast32_t i = 0;
                while (i != size && m_data[i] != f)
                {
                        ++i;
                }
                if (i != size)
                {
                        std::memmove(&m_data[i], &m_data[i + 1], sizeof(f) * (size - (i + 1)));
                        m_data.resize(size - 1);
                }
        }
#endif
        void erase(const T* f)
        {
                int size = m_data.size();
                const T* next = nullptr;
                for (int_fast32_t i = size - 1; i >= 0; --i)
                {
                        const T* current = m_data[i];
                        m_data[i] = next;
                        if (current != f)
                        {
                                next = current;
                        }
                        else
                        {
                                m_data.resize(size - 1);
                                return;
                        }
                }
                error("facet not found in facets of point");
        }
        size_t size() const
        {
                return m_data.size();
        }
        void clear()
        {
                m_data.clear();
                // m_data.shrink_to_fit();
        }
        typename std::vector<const T*>::const_iterator begin() const
        {
                return m_data.cbegin();
        }
        typename std::vector<const T*>::const_iterator end() const
        {
                return m_data.cend();
        }
};

template <unsigned simplex_i, size_t N, typename SourceType, typename ComputeType>
void find_simplex_points(const std::vector<Vector<N, SourceType>>& points, std::array<int, N + 1>* simplex_points,
                         std::array<Vector<N, ComputeType>, N>* simplex_vectors, unsigned point_i)
{
        static_assert(N > 1);
        static_assert(simplex_i <= N);

        for (; point_i < points.size(); ++point_i)
        {
                minus(&(*simplex_vectors)[simplex_i - 1], points[point_i], points[(*simplex_points)[0]]);

                if (linearly_independent<simplex_i>(*simplex_vectors))
                {
                        break;
                }
        }

        if (point_i == points.size())
        {
                error("point " + to_string(simplex_i + 1) + " of " + to_string(N) + "-simplex not found");
        }

        (*simplex_points)[simplex_i] = point_i;

        // N - максимальный индекс для массива из N + 1 точек
        if constexpr (simplex_i != N)
        {
                find_simplex_points<simplex_i + 1>(points, simplex_points, simplex_vectors, point_i + 1);
        }
};

template <size_t N, typename SourceType, typename ComputeType>
void find_simplex_points(const std::vector<Vector<N, SourceType>>& points, std::array<int, N + 1>* simplex_points)
{
        static_assert(N > 1);

        if (points.size() == 0)
        {
                error("0-simplex not found");
        }

        std::array<Vector<N, ComputeType>, N> simplex_vectors;

        (*simplex_points)[0] = 0;

        find_simplex_points<1>(points, simplex_points, &simplex_vectors, 1);
}

template <size_t N, typename Facet, template <typename...> typename Map>
void connect_facets(Facet* facet, int exclude_point, Map<Ridge<N>, std::tuple<Facet*, unsigned>>* search_map, int* ridge_count)
{
        const std::array<int, N>& vertices = facet->vertices();
        for (unsigned r = 0; r < N; ++r)
        {
                if (vertices[r] == exclude_point)
                {
                        // Ребро горизонта, грань с ним уже соединена при её создании
                        continue;
                }

                Ridge<N> ridge(del_elem(vertices, r));

                auto search_iter = search_map->find(ridge);
                if (search_iter == search_map->end())
                {
                        search_map->emplace(std::move(ridge), std::make_tuple(facet, r));
                }
                else
                {
                        Facet* link_facet = std::get<0>(search_iter->second);
                        unsigned link_r = std::get<1>(search_iter->second);

                        facet->set_link(r, link_facet);
                        link_facet->set_link(link_r, facet);

                        search_map->erase(search_iter);

                        ++(*ridge_count);
                }
        }
}

// Найти N + 1 вершин N-симплекса — начальной выпуклой оболочки N-мерного пространства.
template <size_t N, typename S, typename C>
void create_init_convex_hull(const std::vector<Vector<N, S>>& points, std::array<int, N + 1>* vertices,
                             FacetList<Facet<N, S, C>>* facets)
{
        find_simplex_points<N, S, C>(points, vertices);

        // Выпуклая оболочка из найденных N + 1 вершин состоит из N + 1 граней,
        // что равно количеству сочетаний по N вершины из N + 1 вершин.
        facets->clear();
        for (unsigned f = 0; f < N + 1; ++f)
        {
                facets->emplace_back(points, del_elem(*vertices, f), (*vertices)[f], nullptr);
                std::prev(facets->end())->set_iter(std::prev(facets->cend()));
        }

        // Количество рёбер для N-симплекса равно количеству
        // сочетаний по N - 1 вершины из N + 1 вершин.
        constexpr int ridge_count = binomial(N + 1, N - 1);

        int ridges = 0;
        std::unordered_map<Ridge<N>, std::tuple<Facet<N, S, C>*, unsigned>> search_map(ridge_count);
        for (Facet<N, S, C>& facet : *facets)
        {
                connect_facets(&facet, -1, &search_map, &ridges);
        }
        ASSERT(search_map.size() == 0);
        ASSERT(ridges == ridge_count);
}

// Начальное заполнение списков конфликтов граней и точек
template <typename Point, typename Facet>
void create_init_conflict_lists(const std::vector<Point>& points, const std::vector<unsigned char>& enabled,
                                FacetList<Facet>* facets, std::vector<FacetStore<Facet>>* point_conflicts)
{
        for (Facet& facet : *facets)
        {
                for (unsigned point = 0; point < points.size(); ++point)
                {
                        if (enabled[point] && facet.visible_from_point(points, point))
                        {
                                (*point_conflicts)[point].insert(&facet);
                                facet.add_conflict_point(point);
                        }
                }
        }
}

template <typename Point, typename Facet>
void add_conflict_points_to_new_facet(const std::vector<Point>* points, int point, std::vector<signed char>* unique_points,
                                      const Facet* facet_0, const Facet* facet_1, Facet* new_facet)
{
        for (int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 1;

                if (p != point && new_facet->visible_from_point(*points, p))
                {
                        new_facet->add_conflict_point(p);
                }
        }
        for (int p : facet_1->conflict_points())
        {
                if ((*unique_points)[p] != 0)
                {
                        continue;
                }

                if (p != point && new_facet->visible_from_point(*points, p))
                {
                        new_facet->add_conflict_point(p);
                }
        }
        for (int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 0;
        }
}

template <typename Facet>
void erase_visible_facets_from_conflict_points(unsigned thread_id, unsigned thread_count,
                                               std::vector<FacetStore<Facet>>* point_conflicts, int point)
{
        for (const Facet* facet : (*point_conflicts)[point])
        {
                for (int p : facet->conflict_points())
                {
                        if (p != point && (p % thread_count) == thread_id)
                        {
                                (*point_conflicts)[p].erase(facet);
                        }
                }
        }
}

template <typename Facet>
void add_new_facets_to_conflict_points(unsigned thread_id, unsigned thread_count,
                                       std::vector<FacetList<Facet>>* new_facets_vector,
                                       std::vector<FacetStore<Facet>>* point_conflicts)
{
        for (unsigned i = 0; i < new_facets_vector->size(); ++i)
        {
                for (const Facet& facet : (*new_facets_vector)[i])
                {
                        for (int p : facet.conflict_points())
                        {
                                if ((p % thread_count) == thread_id)
                                {
                                        (*point_conflicts)[p].insert(&facet);
                                }
                        }
                }
        }
}

template <typename Point, typename Facet>
void create_facets(unsigned thread_id, unsigned thread_count, const std::vector<Point>* points, int point,
                   std::vector<FacetStore<Facet>>* point_conflicts, std::vector<std::vector<signed char>>* unique_points_work,
                   std::vector<FacetList<Facet>>* new_facets_vector)
{
        ASSERT(new_facets_vector->size() == thread_count);
        ASSERT(unique_points_work->size() == thread_count);

        std::vector<signed char>* unique_points = &(*unique_points_work)[thread_id];
        FacetList<Facet>* new_facets = &(*new_facets_vector)[thread_id];

        new_facets->clear();

        unsigned ridge_count = 0;

        // Добавление граней, состоящих из рёбер горизонта и заданной точки
        for (const Facet* facet : (*point_conflicts)[point])
        {
                for (unsigned r = 0; r < facet->vertices().size(); ++r)
                {
                        Facet* link_facet = facet->get_link(r);

                        if (link_facet->marked_as_visible())
                        {
                                continue;
                        }
                        if (ridge_count != thread_id)
                        {
                                ++ridge_count;
                                continue;
                        }
                        ++ridge_count;
                        thread_id += thread_count;

                        // Создание новой грани и соединение новой грани с горизонтом вместо этой грани

                        int link_index = link_facet->find_link_index(facet);

                        new_facets->emplace_back(*points, set_elem(facet->vertices(), r, point),
                                                 link_facet->vertices()[link_index], link_facet);

                        Facet* new_facet = &(*std::prev(new_facets->end()));
                        new_facet->set_iter(std::prev(new_facets->cend()));

                        new_facet->set_link(new_facet->find_index_for_point(point), link_facet);
                        link_facet->set_link(link_index, new_facet);

                        add_conflict_points_to_new_facet(points, point, unique_points, facet, link_facet, new_facet);
                }
        }
}

// Используются указатели "const type*", так как при ссылках "const type&" происходит копирование данных
// при работе с std::function и std::thread
template <size_t N, typename S, typename C>
void create_horizon_facets(unsigned thread_id, unsigned thread_count, const std::vector<Vector<N, S>>* points, int point,
                           std::vector<FacetStore<Facet<N, S, C>>>* point_conflicts,
                           std::vector<std::vector<signed char>>* unique_points_work,
                           std::vector<FacetList<Facet<N, S, C>>>* new_facets_vector, ThreadBarrier* thread_barrier)
{
        try
        {
                create_facets(thread_id, thread_count, points, point, point_conflicts, unique_points_work, new_facets_vector);
        }
        catch (...)
        {
                thread_barrier->wait();
                throw;
        }
        thread_barrier->wait();

        // Вначале убрать ссылки на видимые грани, а потом добавить ссылки на новые грани.
        // Это нужно для уменьшения объёма поиска граней у точек.

        erase_visible_facets_from_conflict_points(thread_id, thread_count, point_conflicts, point);

        add_new_facets_to_conflict_points(thread_id, thread_count, new_facets_vector, point_conflicts);
}

template <typename T>
unsigned calculate_facet_count(const std::vector<T>& facets)
{
        return std::accumulate(facets.cbegin(), facets.cend(), 0, [](unsigned a, const T& b) { return a + b.size(); });
}

template <size_t N, typename S, typename C>
void add_point_to_convex_hull(const std::vector<Vector<N, S>>& points, int point, FacetList<Facet<N, S, C>>* facets,
                              std::vector<FacetStore<Facet<N, S, C>>>* point_conflicts, ThreadPool* thread_pool,
                              ThreadBarrier* thread_barrier, std::vector<std::vector<signed char>>* unique_points_work)
{
        if ((*point_conflicts)[point].size() == 0)
        {
                // точка находится внутри оболочки
                return;
        }

        if ((*point_conflicts)[point].size() >= facets->size())
        {
                error("All facets are visible from the point");
        }

        for (const Facet<N, S, C>* facet : (*point_conflicts)[point])
        {
                facet->mark_as_visible();
        }

        std::vector<FacetList<Facet<N, S, C>>> new_facets(thread_pool->thread_count());

        // Добавление граней, состоящих из рёбер горизонта и заданной точки.
        if (thread_pool->thread_count() > 1)
        {
                thread_pool->run([&](unsigned thread_id, unsigned thread_count) {
                        create_horizon_facets(thread_id, thread_count, &points, point, point_conflicts, unique_points_work,
                                              &new_facets, thread_barrier);
                });
        }
        else
        {
                // 0 = thread_id, 1 = thread_count
                create_horizon_facets(0, 1, &points, point, point_conflicts, unique_points_work, &new_facets, thread_barrier);
        }

        // Удаление видимых граней
        for (const Facet<N, S, C>* facet : (*point_conflicts)[point])
        {
                facets->erase(facet->get_iter());
        }

        // Для этой точки больше не нужен список видимых граней
        (*point_conflicts)[point].clear();

        int facet_count = calculate_facet_count(new_facets);
        int ridge_count = (N - 1) * facet_count / 2;

        // Соединить новые грани между собой, кроме граней горизонта
        int ridges = 0;
        std::unordered_map<Ridge<N>, std::tuple<Facet<N, S, C>*, unsigned>> search_map(ridge_count);
        for (unsigned i = 0; i < new_facets.size(); ++i)
        {
                for (Facet<N, S, C>& facet : new_facets[i])
                {
                        connect_facets(&facet, point, &search_map, &ridges);
                }
        }
        ASSERT(search_map.size() == 0);
        ASSERT(ridges == ridge_count);

        // Добавить новые грани в общий список граней
        for (unsigned i = 0; i < new_facets.size(); ++i)
        {
                facets->splice(facets->cend(), new_facets[i]);
        }
}

template <size_t N, typename S, typename C>
void create_convex_hull(const std::vector<Vector<N, S>>& points, FacetList<Facet<N, S, C>>* facets, ProgressRatio* progress)
{
        static_assert(N > 1);

        // Проверить на минимум по количеству точек
        if (points.size() < N + 1)
        {
                error("Error point count " + to_string(points.size()) + " for convex hull in " + space_name(N));
        }

        facets->clear();

        std::array<int, N + 1> init_vertices;

        create_init_convex_hull(points, &init_vertices, facets);

        std::vector<unsigned char> point_enabled(points.size(), true);
        for (int v : init_vertices)
        {
                point_enabled[v] = false;
        }

        std::vector<FacetStore<Facet<N, S, C>>> point_conflicts(points.size());

        create_init_conflict_lists(points, point_enabled, facets, &point_conflicts);

        ThreadPool thread_pool(thread_count<S, C>());
        ThreadBarrier thread_barrier(thread_pool.thread_count());

        // Создаётся здесь, чтобы каждый раз не создавать при расчёте и не выделять каждый раз память,
        // а также не использовать thread_local
        std::vector<std::vector<signed char>> unique_points_work(thread_pool.thread_count());
        for (unsigned i = 0; i < unique_points_work.size(); ++i)
        {
                unique_points_work[i].resize(points.size(), 0);
        }

        // N-симплекс построен, значит уже обработано N + 1 точек
        for (unsigned i = 0, points_done = N + 1; i < points.size(); ++i, ++points_done)
        {
                if (!point_enabled[i])
                {
                        continue;
                }

                if (ProgressRatio::lock_free())
                {
                        progress->set(points_done, points.size());
                }

                add_point_to_convex_hull(points, i, facets, &point_conflicts, &thread_pool, &thread_barrier, &unique_points_work);
        }

        ASSERT(std::all_of(facets->cbegin(), facets->cend(),
                           [](const Facet<N, S, C>& facet) -> bool { return facet.conflict_points().size() == 0; }));
}

template <size_t N>
void find_min_max(const std::vector<Vector<N, float>>& points, Vector<N, float>* min, Vector<N, float>* max)
{
        ASSERT(points.size() > 0);

        *min = *max = points[0];
        for (unsigned i = 1; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        (*min)[n] = std::min((*min)[n], points[i][n]);
                        (*max)[n] = std::max((*max)[n], points[i][n]);
                }
        }
}

// Для алгоритма принципиально не нужны одинаковые точки - разность между ними даст нулевой вектор.
// Для алгоритма со списками конфликтов принципиально нужен случайный порядок обработки точек.
// Масштабирование и перевод в целые числа с сохранением пропорций.
template <size_t N>
void shuffle_and_convert_to_unique_integer(const std::vector<Vector<N, float>>& source_points, long long max_value,
                                           std::vector<Vector<N, long long>>* points, std::vector<int>* points_map)
{
        ASSERT(0 < max_value);

        points->clear();
        points->reserve(source_points.size());

        points_map->clear();
        points_map->reserve(source_points.size());

        std::vector<int> random_map(source_points.size());
        std::iota(random_map.begin(), random_map.end(), 0);
        std::shuffle(random_map.begin(), random_map.end(), std::mt19937_64(source_points.size()));

        Vector<N, float> min, max;
        find_min_max(source_points, &min, &max);

        double max_d = max_element(max - min);
        if (max_d == 0)
        {
                error("all points are equal to each other");
        }
        double scale_factor = max_value / max_d;

        std::unordered_set<Vector<N, long long>> unique_check(source_points.size());

        for (unsigned i = 0; i < source_points.size(); ++i)
        {
                int random_i = random_map[i];

                Vector<N, double> float_value = to_vector<double>(source_points[random_i] - min) * scale_factor;

                Vector<N, long long> integer_value;

                for (unsigned n = 0; n < N; ++n)
                {
                        long long ll = std::llround(float_value[n]);
                        if (ll < 0 || ll > max_value)
                        {
                                error("points preprocessing error: value < 0 || value > MAX");
                        }
                        integer_value[n] = ll;
                }

                if (unique_check.count(integer_value) == 0)
                {
                        unique_check.insert(integer_value);
                        points->push_back(integer_value);
                        points_map->push_back(random_i);
                }
        }
}

template <size_t N>
std::array<int, N> restore_indices(const std::array<int, N>& vertices, const std::vector<int>& points_map)
{
        std::array<int, N> res;
        for (unsigned n = 0; n < N; ++n)
        {
                res[n] = points_map[vertices[n]];
        }
        return res;
}

template <size_t N>
void paraboloid_convex_hull(const std::vector<Vector<N, long long>>& points, const std::vector<int>& points_map,
                            std::vector<DelaunaySimplex<N>>* simplices, ProgressRatio* progress)
{
        using FacetCH = Facet<N + 1, DataTypeParaboloid<N + 1>, ComputeTypeParaboloid<N + 1>>;
        using PointCH = Vector<N + 1, DataTypeParaboloid<N + 1>>;

        LOG("Paraboloid " + space_name(N + 1) + ". Max: " + to_string(PARABOLOID_BITS) +
            ". Data: " + type_str<DataTypeParaboloid<N + 1>>() + ". Compute: " + type_str<ComputeTypeParaboloid<N + 1>>() + ". " +
            space_name(N) + ". Data: " + type_str<DataTypeAfterParaboloid<N>>() +
            "; Compute: " + type_str<ComputeTypeAfterParaboloid<N>>());

        // Рассчитать Делоне на основе нижней части выпуклой оболочки параболоида размерности N + 1

        std::vector<PointCH> data(points.size());

        // Размещение точек на параболоиде
        for (unsigned i = 0; i < points.size(); ++i)
        {
                data[i][N] = 0;
                for (unsigned n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                        // умножение делается в типе данных расчёта, а не в типе исходных данных
                        data[i][N] += data[i][n] * data[i][n];
                }
        }

        FacetList<FacetCH> facets;

        create_convex_hull(data, &facets, progress);

        data.clear();
        data.shrink_to_fit();

        // Рассчитать ортогональные дополнения к граням в исходном пространстве
        // размерности N и создать грани

        using FacetDelaunay = Facet<N, DataTypeAfterParaboloid<N>, ComputeTypeAfterParaboloid<N>>;
        using PointDelaunay = Vector<N, DataTypeAfterParaboloid<N>>;

        std::vector<PointDelaunay> data_d(points.size());
        for (unsigned i = 0; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        data_d[i][n] = points[i][n];
                }
        }

        simplices->clear();
        simplices->reserve(facets.size());
        for (const FacetCH& facet : facets)
        {
                // Если последняя координата перпендикуляра грани больше или равна 0,
                // то эта грань не является нижней частью выпуклой оболочки параболоида
                if (!facet.last_ortho_coord_is_negative())
                {
                        continue;
                }

                const std::array<int, N + 1>& vertices = facet.vertices();

                std::array<vec<N>, N + 1> orthos;
                for (unsigned r = 0; r < N + 1; ++r)
                {
                        // Перпендикуляр к грани наружу от симплекса Делоне
                        orthos[r] = FacetDelaunay(data_d, del_elem(vertices, r), vertices[r], nullptr).double_ortho();
                }

                simplices->emplace_back(restore_indices(vertices, points_map), orthos);
        }
}

template <size_t N>
void ordinary_convex_hull(const std::vector<Vector<N, long long>>& points, const std::vector<int>& points_map,
                          std::vector<ConvexHullFacet<N>>* ch_facets, ProgressRatio* progress)
{
        using Facet = Facet<N, DataTypeOrdinary<N>, ComputeTypeOrdinary<N>>;
        using Point = Vector<N, DataTypeOrdinary<N>>;

        LOG(space_name(N) + ".  Max: " + to_string(ORDINARY_BITS) + ". Data: " + type_str<DataTypeOrdinary<N>>() +
            ". Compute: " + type_str<ComputeTypeOrdinary<N>>());

        std::vector<Point> data(points.size());

        for (unsigned i = 0; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                }
        }

        FacetList<Facet> facets;

        create_convex_hull(data, &facets, progress);

        ch_facets->clear();
        ch_facets->reserve(facets.size());
        for (const Facet& facet : facets)
        {
                ch_facets->emplace_back(restore_indices(facet.vertices(), points_map), facet.double_ortho());
        }
}

//
// Подготовка данных для выпуклой оболочки
//

template <size_t N>
void delaunay_integer(const std::vector<Vector<N, float>>& source_points, std::vector<vec<N>>* points,
                      std::vector<DelaunaySimplex<N>>* simplices, ProgressRatio* progress)
{
        LOG("convex hull paraboloid in " + space_name(N + 1) + " integer");

        std::vector<Vector<N, long long>> convex_hull_points;
        std::vector<int> points_map;

        constexpr long long MAX = (1ull << PARABOLOID_BITS) - 1;

        shuffle_and_convert_to_unique_integer(source_points, MAX, &convex_hull_points, &points_map);

        paraboloid_convex_hull(convex_hull_points, points_map, simplices, progress);

        points->clear();
        points->resize(source_points.size(), vec<N>(0));
        for (unsigned i = 0; i < convex_hull_points.size(); ++i)
        {
                (*points)[points_map[i]] = to_vector<double>(convex_hull_points[i]);
        }

        LOG("convex hull paraboloid in " + space_name(N + 1) + " integer done");
}

template <size_t N>
void convex_hull_integer(const std::vector<Vector<N, float>>& source_points, std::vector<ConvexHullFacet<N>>* facets,
                         ProgressRatio* progress)
{
        LOG("convex hull in " + space_name(N) + " integer");

        std::vector<int> points_map;
        std::vector<Vector<N, long long>> convex_hull_points;

        constexpr long long MAX = (1ull << ORDINARY_BITS) - 1;

        shuffle_and_convert_to_unique_integer(source_points, MAX, &convex_hull_points, &points_map);

        ordinary_convex_hull(convex_hull_points, points_map, facets, progress);

        LOG("convex hull in " + space_name(N) + " integer done");
}
}

template <size_t N>
void compute_delaunay(const std::vector<Vector<N, float>>& source_points, std::vector<vec<N>>* points,
                      std::vector<DelaunaySimplex<N>>* simplices, ProgressRatio* progress)
{
        if (source_points.size() == 0)
        {
                error("no points for convex hull");
        }

        delaunay_integer(source_points, points, simplices, progress);
}

template <size_t N>
void compute_convex_hull(const std::vector<Vector<N, float>>& source_points, std::vector<ConvexHullFacet<N>>* ch_facets,
                         ProgressRatio* progress)
{
        if (source_points.size() == 0)
        {
                error("no points for convex hull");
        }

        convex_hull_integer(source_points, ch_facets, progress);
}

//

template void compute_delaunay(const std::vector<Vector<2, float>>& source_points, std::vector<vec<2>>* points,
                               std::vector<DelaunaySimplex<2>>* simplices, ProgressRatio* progress);
template void compute_delaunay(const std::vector<Vector<3, float>>& source_points, std::vector<vec<3>>* points,
                               std::vector<DelaunaySimplex<3>>* simplices, ProgressRatio* progress);
template void compute_delaunay(const std::vector<Vector<4, float>>& source_points, std::vector<vec<4>>* points,
                               std::vector<DelaunaySimplex<4>>* simplices, ProgressRatio* progress);
template void compute_delaunay(const std::vector<Vector<5, float>>& source_points, std::vector<vec<5>>* points,
                               std::vector<DelaunaySimplex<5>>* simplices, ProgressRatio* progress);

template void compute_convex_hull(const std::vector<Vector<2, float>>& source_points, std::vector<ConvexHullFacet<2>>* ch_facets,
                                  ProgressRatio* progress);
template void compute_convex_hull(const std::vector<Vector<3, float>>& source_points, std::vector<ConvexHullFacet<3>>* ch_facets,
                                  ProgressRatio* progress);
template void compute_convex_hull(const std::vector<Vector<4, float>>& source_points, std::vector<ConvexHullFacet<4>>* ch_facets,
                                  ProgressRatio* progress);
template void compute_convex_hull(const std::vector<Vector<5, float>>& source_points, std::vector<ConvexHullFacet<5>>* ch_facets,
                                  ProgressRatio* progress);
template void compute_convex_hull(const std::vector<Vector<6, float>>& source_points, std::vector<ConvexHullFacet<6>>* ch_facets,
                                  ProgressRatio* progress);
