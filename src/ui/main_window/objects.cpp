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

#include "objects.h"

#include "catch.h"
#include "identifiers.h"
#include "meshes.h"

#include "com/error.h"
#include "com/log.h"
#include "com/mat.h"
#include "com/names.h"
#include "com/time.h"
#include "com/variant.h"
#include "geometry/cocone/reconstruction.h"
#include "geometry/graph/mst.h"
#include "geometry/objects/points.h"
#include "obj/obj.h"
#include "obj/obj_alg.h"
#include "obj/obj_convex_hull.h"
#include "obj/obj_facets.h"
#include "obj/obj_file.h"
#include "obj/obj_file_load.h"
#include "obj/obj_file_save.h"
#include "obj/obj_lines.h"
#include "obj/obj_points.h"
#include "path_tracing/shapes/mesh.h"
#include "progress/progress.h"

#include <mutex>
#include <thread>
#include <unordered_map>

constexpr int MinDimension = 3;
constexpr int MaxDimension = 5;

template <size_t N>
class MainObjectsImpl
{
        static_assert(N >= 3);

        enum class ObjectType
        {
                Model,
                Cocone,
                BoundCocone
        };

        const std::thread::id m_thread_id = std::this_thread::get_id();
        const int m_mesh_threads;

        const WindowEventEmitter& m_event_emitter;

        //

        const std::unique_ptr<IObjectRepository<N>> m_object_repository;
        Meshes<int, const Mesh<N, double>> m_meshes;
        Meshes<int, const Obj<N>> m_objects;
        std::vector<Vector<N, float>> m_manifold_points;
        std::unique_ptr<IManifoldConstructor<N>> m_manifold_constructor;
        Matrix<N + 1, N + 1, double> m_model_vertex_matrix;

        //

        std::mutex m_mesh_sequential_mutex;

        double m_bound_cocone_rho;
        double m_bound_cocone_alpha;

        IShow* m_show;

        template <typename F>
        void catch_all(const F& function) const noexcept;

        static std::string object_name(ObjectType object_type);
        static int object_identifier(ObjectType object_type);
        static int convex_hull_identifier(ObjectType object_type);

        void build_mst(ProgressRatioList* progress_list);
        void build_mesh(ProgressRatioList* progress_list, int id, const Obj<N>& obj);
        void add_object_and_convex_hull(ProgressRatioList* progress_list, ObjectType object_type,
                                        const std::shared_ptr<const Obj<N>>& obj);
        void object_and_mesh(ProgressRatioList* progress_list, ObjectType object_type, const std::shared_ptr<const Obj<N>>& obj);
        void manifold_constructor(ProgressRatioList* progress_list, double rho, double alpha);
        void cocone(ProgressRatioList* progress_list);
        void bound_cocone(ProgressRatioList* progress_list, double rho, double alpha);

        template <typename ObjectLoaded>
        void load_object(ProgressRatioList* progress_list, const std::string& object_name,
                         const std::shared_ptr<const Obj<N>>& obj, double rho, double alpha, const ObjectLoaded& object_loaded);

public:
        MainObjectsImpl(int mesh_threads, const WindowEventEmitter& emitter);

        void clear_all_data() noexcept;

        std::vector<std::string> repository_point_object_names() const;

        void set_show(IShow* show);

        bool manifold_constructor_exists() const;
        bool object_exists(int id) const;
        bool mesh_exists(int id) const;

        void compute_bound_cocone(ProgressRatioList* progress_list, double rho, double alpha);

        template <typename ObjectLoaded>
        void load_from_file(ProgressRatioList* progress_list, const std::string& file_name, double rho, double alpha,
                            const ObjectLoaded& object_loaded);

        template <typename ObjectLoaded>
        void load_from_repository(ProgressRatioList* progress_list, const std::string& object_name, double rho, double alpha,
                                  int point_count, const ObjectLoaded& object_loaded);

        void save_to_file(int id, const std::string& file_name, const std::string& name) const;
        void paint(int id, const PaintingInformation3d& info_3d, const PaintingInformationNd& info_nd,
                   const PaintingInformationAll& info_all) const;
};

template <size_t N>
MainObjectsImpl<N>::MainObjectsImpl(int mesh_threads, const WindowEventEmitter& emitter)
        : m_mesh_threads(mesh_threads),
          m_event_emitter(emitter),
          m_object_repository(create_object_repository<N>()),
          m_show(nullptr)
{
}

template <size_t N>
template <typename F>
void MainObjectsImpl<N>::catch_all(const F& function) const noexcept
{
        static_assert(noexcept(catch_all_exceptions(m_event_emitter, function)));

        catch_all_exceptions(m_event_emitter, function);
}

template <size_t N>
std::vector<std::string> MainObjectsImpl<N>::repository_point_object_names() const
{
        return m_object_repository->point_object_names();
}

template <size_t N>
void MainObjectsImpl<N>::set_show(IShow* show)
{
        m_show = show;
}

template <size_t N>
bool MainObjectsImpl<N>::object_exists(int id) const
{
        return m_objects.get(id) != nullptr;
}

template <size_t N>
bool MainObjectsImpl<N>::mesh_exists(int id) const
{
        return m_meshes.get(id) != nullptr;
}

template <size_t N>
bool MainObjectsImpl<N>::manifold_constructor_exists() const
{
        return m_manifold_constructor.get() != nullptr;
}

template <size_t N>
std::string MainObjectsImpl<N>::object_name(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return "Model";
        case ObjectType::Cocone:
                return "Cocone";
        case ObjectType::BoundCocone:
                return "BoundCocone";
        }
        error_fatal("Unknown object type");
}

template <size_t N>
int MainObjectsImpl<N>::object_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return OBJECT_MODEL;
        case ObjectType::Cocone:
                return OBJECT_COCONE;
        case ObjectType::BoundCocone:
                return OBJECT_BOUND_COCONE;
        };
        error_fatal("Unknown object type");
}

template <size_t N>
int MainObjectsImpl<N>::convex_hull_identifier(ObjectType object_type)
{
        switch (object_type)
        {
        case ObjectType::Model:
                return OBJECT_MODEL_CONVEX_HULL;
        case ObjectType::Cocone:
                return OBJECT_COCONE_CONVEX_HULL;
        case ObjectType::BoundCocone:
                return OBJECT_BOUND_COCONE_CONVEX_HULL;
        };
        error_fatal("Unknown object type");
}

template <size_t N>
void MainObjectsImpl<N>::build_mesh(ProgressRatioList* progress_list, int id, const Obj<N>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (obj.facets().size() == 0)
        {
                return;
        }

        std::lock_guard lg(m_mesh_sequential_mutex);

        ProgressRatio progress(progress_list);

        m_meshes.set(id, std::make_shared<const Mesh<N, double>>(&obj, m_model_vertex_matrix, m_mesh_threads, &progress));
}

template <size_t N>
void MainObjectsImpl<N>::add_object_and_convex_hull(ProgressRatioList* progress_list, ObjectType object_type,
                                                    const std::shared_ptr<const Obj<N>>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (!(obj->facets().size() > 0 || (object_type == ObjectType::Model && obj->points().size() > 0)))
        {
                return;
        }

        int object_id = object_identifier(object_type);
        if constexpr (N == 3)
        {
                m_show->add_object(obj, object_id, OBJECT_MODEL);
        }
        m_objects.set(object_id, obj);

        std::shared_ptr<const Obj<N>> convex_hull;

        {
                ProgressRatio progress(progress_list);
                progress.set_text(object_name(object_type) + " convex hull in " + space_name(N) + ": %v of %m");

                convex_hull = create_convex_hull_for_obj(obj.get(), &progress);
        }

        if (convex_hull->facets().size() > 0)
        {
                int convex_hull_id = convex_hull_identifier(object_type);
                if constexpr (N == 3)
                {
                        m_show->add_object(convex_hull, convex_hull_id, OBJECT_MODEL);
                }
                m_objects.set(convex_hull_id, convex_hull);
        }

        build_mesh(progress_list, convex_hull_identifier(object_type), *convex_hull);
}

template <size_t N>
void MainObjectsImpl<N>::object_and_mesh(ProgressRatioList* progress_list, ObjectType object_type,
                                         const std::shared_ptr<const Obj<N>>& obj)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::thread thread_model([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " object and convex hull";

                        add_object_and_convex_hull(progress_list, object_type, obj);
                });
        });

        std::thread thread_mesh([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = object_name(object_type) + " mesh";

                        build_mesh(progress_list, object_identifier(object_type), *obj);
                });
        });

        thread_model.join();
        thread_mesh.join();
}

template <size_t N>
void MainObjectsImpl<N>::cocone(ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->cocone(&normals, &facets, &progress);

                obj_cocone = create_obj_for_facets(m_manifold_points, normals, facets);

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        object_and_mesh(progress_list, ObjectType::Cocone, obj_cocone);
}

template <size_t N>
void MainObjectsImpl<N>::bound_cocone(ProgressRatioList* progress_list, double rho, double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (!m_manifold_constructor)
        {
                error("No manifold constructor");
        }

        std::shared_ptr<const Obj<N>> obj_bound_cocone;

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                std::vector<vec<N>> normals;
                std::vector<std::array<int, N>> facets;

                m_manifold_constructor->bound_cocone(rho, alpha, &normals, &facets, &progress);

                obj_bound_cocone = create_obj_for_facets(m_manifold_points, normals, facets);

                m_bound_cocone_rho = rho;
                m_bound_cocone_alpha = alpha;

                LOG("Manifold reconstruction second phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        if constexpr (N == 3)
        {
                m_show->delete_object(OBJECT_BOUND_COCONE);
                m_show->delete_object(OBJECT_BOUND_COCONE_CONVEX_HULL);
        }

        m_meshes.reset(OBJECT_BOUND_COCONE);
        m_meshes.reset(OBJECT_BOUND_COCONE_CONVEX_HULL);
        m_objects.reset(OBJECT_BOUND_COCONE);
        m_objects.reset(OBJECT_BOUND_COCONE_CONVEX_HULL);

        m_event_emitter.bound_cocone_loaded(rho, alpha);

        object_and_mesh(progress_list, ObjectType::BoundCocone, obj_bound_cocone);
}

template <size_t N>
void MainObjectsImpl<N>::build_mst(ProgressRatioList* progress_list)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::vector<std::array<int, 2>> mst_lines;

        {
                ProgressRatio progress(progress_list);

                mst_lines = minimum_spanning_tree(m_manifold_points, m_manifold_constructor->delaunay_objects(), &progress);
        }

        std::shared_ptr<const Obj<N>> mst_obj = create_obj_for_lines(m_manifold_points, mst_lines);

        if (mst_obj->lines().size() > 0)
        {
                if constexpr (N == 3)
                {
                        m_show->add_object(mst_obj, OBJECT_MODEL_MST, OBJECT_MODEL);
                }
                m_objects.set(OBJECT_MODEL_MST, mst_obj);
        }
}

template <size_t N>
void MainObjectsImpl<N>::manifold_constructor(ProgressRatioList* progress_list, double rho, double alpha)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        {
                ProgressRatio progress(progress_list);

                double start_time = time_in_seconds();

                m_manifold_constructor = create_manifold_constructor(m_manifold_points, &progress);

                LOG("Manifold reconstruction first phase, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
        }

        std::thread thread_cocone([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Cocone reconstruction in " + space_name(N);

                        cocone(progress_list);
                });
        });

        std::thread thread_bound_cocone([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "BoundCocone reconstruction in " + space_name(N);

                        bound_cocone(progress_list, rho, alpha);
                });
        });

        std::thread thread_mst([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Minimum spanning tree in " + space_name(N);

                        build_mst(progress_list);
                });
        });

        thread_cocone.join();
        thread_bound_cocone.join();
        thread_mst.join();
}

template <size_t N>
void MainObjectsImpl<N>::clear_all_data() noexcept
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if constexpr (N == 3)
        {
                m_show->delete_all_objects();
        }
        m_manifold_constructor.reset();
        m_meshes.reset_all();
        m_objects.reset_all();
        m_manifold_points.clear();
        m_manifold_points.shrink_to_fit();
}

template <size_t N>
template <typename ObjectLoaded>
void MainObjectsImpl<N>::load_object(ProgressRatioList* progress_list, const std::string& object_name,
                                     const std::shared_ptr<const Obj<N>>& obj, double rho, double alpha,
                                     const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        if (obj->facets().size() == 0 && obj->points().size() == 0)
        {
                error("Facets or points not found");
        }

        if (obj->facets().size() != 0 && obj->points().size() != 0)
        {
                error("Facets and points together in one object are not supported");
        }

        // Вызов object_loaded() предназначен для вызовов
        // clear_all_data() для всех объектов всех измерений
        object_loaded();

        m_event_emitter.file_loaded(object_name, N);

        m_manifold_points = (obj->facets().size() > 0) ? unique_facet_vertices(obj.get()) : unique_point_vertices(obj.get());

        if constexpr (N == 3)
        {
                m_model_vertex_matrix = model_vertex_matrix(obj.get(), m_show->object_size(), m_show->object_position());
        }
        else
        {
                m_model_vertex_matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::thread thread_model([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Object and mesh";

                        object_and_mesh(progress_list, ObjectType::Model, obj);
                });
        });

        std::thread thread_manifold([&]() noexcept {
                catch_all([&](std::string* message) {
                        *message = "Manifold constructor";

                        manifold_constructor(progress_list, rho, alpha);
                });
        });

        thread_model.join();
        thread_manifold.join();
}

template <size_t N>
void MainObjectsImpl<N>::compute_bound_cocone(ProgressRatioList* progress_list, double rho, double alpha)
{
        bound_cocone(progress_list, rho, alpha);
}

template <size_t N>
template <typename ObjectLoaded>
void MainObjectsImpl<N>::load_from_file(ProgressRatioList* progress_list, const std::string& file_name, double rho, double alpha,
                                        const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Load file: %p%");
                obj = load_obj_from_file<N>(file_name, &progress);
        }

        load_object(progress_list, file_name, obj, rho, alpha, object_loaded);
}

template <size_t N>
template <typename ObjectLoaded>
void MainObjectsImpl<N>::load_from_repository(ProgressRatioList* progress_list, const std::string& object_name, double rho,
                                              double alpha, int point_count, const ObjectLoaded& object_loaded)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj;

        {
                ProgressRatio progress(progress_list);

                progress.set_text("Load object: %p%");
                obj = create_obj_for_points(m_object_repository->point_object(object_name, point_count));
        }

        load_object(progress_list, object_name, obj, rho, alpha, object_loaded);
}

template <size_t N>
void MainObjectsImpl<N>::save_to_file(int id, const std::string& file_name, const std::string& name) const
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::shared_ptr<const Obj<N>> obj = m_objects.get(id);

        if (!obj)
        {
                m_event_emitter.message_warning("No object to export");
                return;
        }

        save_obj_geometry_to_file(obj.get(), file_name, name);
}

template <size_t N>
void MainObjectsImpl<N>::paint(int id, const PaintingInformation3d& info_3d, const PaintingInformationNd& info_nd,
                               const PaintingInformationAll& info_all) const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::shared_ptr<const Mesh<N, double>> mesh = m_meshes.get(id);

        if (!mesh)
        {
                m_event_emitter.message_warning("No object to paint");
                return;
        }

        if constexpr (N == 3)
        {
                painting(mesh, info_3d, info_all);
        }
        else
        {
                painting(mesh, info_nd, info_all);
        }
}

//

template <int Min, int Max>
class MainObjectStorage final : public MainObjects
{
        static_assert(Min >= 3 && Min <= Max);

        static constexpr size_t Count = Max - Min + 1;

        //
        // Каждый элемент контейнера это variant с типами <T<Min, T2...>, T<Min + 1, T2...>, ...>.
        // По каждому номеру I от Min до Max хранится map[I] = T<I, T2...>.
        // Например, имеется тип Type и числа Min = 3, Max = 5.
        // Тип данных контейнера
        //      variant<Type<3>, Type<4>, Type<5>>
        // Элементы контейнера
        //      map[3] = Type<3>
        //      map[4] = Type<4>
        //      map[5] = Type<5>
        //

        // Объекты поддерживаются только у одного измерения. При загрузке
        // объекта одного измерения все объекты других измерений удаляются.

        template <template <size_t, typename...> typename T, typename... T2>
        using Map = std::unordered_map<int, SequenceVariant<Min, Max, T, T2...>>;

        Map<MainObjectsImpl> m_objects;

        void clear_all_data() noexcept
        {
                for (auto& p : m_objects)
                {
                        visit([&](auto& v) noexcept { v.clear_all_data(); }, p.second);
                }
        }

        void check_dimension(int dimension)
        {
                if (dimension < Min || dimension > Max)
                {
                        error("Error repository object dimension " + to_string(dimension) + ", min = " + to_string(Min) +
                              ", max = " + to_string(Max));
                }
        }

        std::vector<std::tuple<int, std::vector<std::string>>> repository_point_object_names() const override
        {
                // Количество измерений, названия объектов
                std::vector<std::tuple<int, std::vector<std::string>>> list;

                for (const auto& p : m_objects)
                {
                        visit([&](const auto& v) { list.push_back({p.first, v.repository_point_object_names()}); }, p.second);
                }

                std::sort(list.begin(), list.end(),
                          [](const std::tuple<int, std::vector<std::string>>& a,
                             const std::tuple<int, std::vector<std::string>>& b) { return std::get<0>(a) < std::get<0>(b); });

                return list;
        }

        void set_show(IShow* show) override
        {
                for (auto& p : m_objects)
                {
                        visit([&](auto& v) { v.set_show(show); }, p.second);
                }
        }

        bool manifold_constructor_exists() const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        visit([&](const auto& v) { count += v.manifold_constructor_exists() ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many manifold constructors " + to_string(count));
                }
                return count > 0;
        }

        bool object_exists(int id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        visit([&](const auto& v) { count += v.object_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many objects " + to_string(count));
                }
                return count > 0;
        }

        bool mesh_exists(int id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        visit([&](const auto& v) { count += v.mesh_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many meshes " + to_string(count));
                }
                return count > 0;
        }

        void compute_bound_cocone(ProgressRatioList* progress_list, double rho, double alpha) override
        {
                if (!manifold_constructor_exists())
                {
                        error("No manifold constructor");
                }
                int count = 0;
                for (auto& p : m_objects)
                {
                        visit(
                                [&](auto& v) {
                                        if (v.manifold_constructor_exists())
                                        {
                                                if (count == 0)
                                                {
                                                        v.compute_bound_cocone(progress_list, rho, alpha);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error manifold constructor count " + to_string(count));
                }
        }

        void load_from_file(ProgressRatioList* progress_list, const std::string& file_name, double rho, double alpha) override
        {
                int dimension = std::get<0>(obj_file_dimension_and_type(file_name));

                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() noexcept
                {
                        clear_all_data();
                };
                visit([&](auto& v) { v.load_from_file(progress_list, file_name, rho, alpha, clear_function); }, repository);
        }

        void load_from_repository(ProgressRatioList* progress_list, const std::tuple<int, std::string>& object, double rho,
                                  double alpha, int point_count) override
        {
                size_t dimension = std::get<0>(object);

                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() noexcept
                {
                        clear_all_data();
                };
                visit(
                        [&](auto& v) {
                                v.load_from_repository(progress_list, std::get<1>(object), rho, alpha, point_count,
                                                       clear_function);
                        },
                        repository);
        }

        void save_to_file(int id, const std::string& file_name, const std::string& name) const override
        {
                if (!object_exists(id))
                {
                        error("No object");
                }
                int count = 0;
                for (const auto& p : m_objects)
                {
                        visit(
                                [&](const auto& v) {
                                        if (v.object_exists(id))
                                        {
                                                if (count == 0)
                                                {
                                                        v.save_to_file(id, file_name, name);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error object count " + to_string(count));
                }
        }

        void paint(int id, const PaintingInformation3d& info_3d, const PaintingInformationNd& info_nd,
                   const PaintingInformationAll& info_all) const override
        {
                if (!mesh_exists(id))
                {
                        error("No mesh");
                }
                int count = 0;
                for (const auto& p : m_objects)
                {
                        visit(
                                [&](const auto& v) {
                                        if (v.mesh_exists(id))
                                        {
                                                if (count == 0)
                                                {
                                                        v.paint(id, info_3d, info_nd, info_all);
                                                }
                                                ++count;
                                        }
                                },
                                p.second);
                }
                if (count != 1)
                {
                        error("Error mesh count " + to_string(count));
                }
        }

        std::string obj_extension(unsigned dimension) const override
        {
                return obj_file_extension(dimension);
        }

        std::vector<std::string> obj_extensions() const override
        {
                std::set<unsigned> dimensions;
                for (unsigned n = Min; n <= Max; ++n)
                {
                        dimensions.insert(n);
                }
                return obj_file_supported_extensions(dimensions);
        }

        std::vector<std::string> txt_extensions() const override
        {
                std::set<unsigned> dimensions;
                for (unsigned n = Min; n <= Max; ++n)
                {
                        dimensions.insert(n);
                }
                return txt_file_supported_extensions(dimensions);
        }

        template <size_t... I>
        void init_map(int mesh_threads, const WindowEventEmitter& emitter, std::integer_sequence<size_t, I...>&&)
        {
                static_assert(((I >= 0 && I < sizeof...(I)) && ...));
                static_assert(Min + sizeof...(I) == Max + 1);

                (m_objects.try_emplace(Min + I, std::in_place_type_t<MainObjectsImpl<Min + I>>(), mesh_threads, emitter), ...);

                ASSERT((m_objects.count(Min + I) == 1) && ...);
                ASSERT(m_objects.size() == Count);
        }

public:
        MainObjectStorage(int mesh_threads, const WindowEventEmitter& emitter)
        {
                init_map(mesh_threads, emitter, std::make_integer_sequence<size_t, Count>());
        }
};

std::unique_ptr<MainObjects> create_main_objects(int mesh_threads, const WindowEventEmitter& emitter)
{
        return std::make_unique<MainObjectStorage<MinDimension, MaxDimension>>(mesh_threads, emitter);
}
