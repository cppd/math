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

#include "storage.h"

#include "space/space.h"

#include <src/com/error.h>
#include <src/model/mesh_utility.h>

namespace
{
constexpr int MIN = STORAGE_MIN_DIMENSIONS;
constexpr int MAX = STORAGE_MAX_DIMENSIONS;
constexpr size_t COUNT = MAX - MIN + 1;

static_assert(MIN >= 3 && MIN <= MAX);
static_assert(std::is_floating_point_v<StorageMeshFloatingPoint>);

class Impl final : public ObjectStorage
{
        //
        // Каждый элемент контейнера это variant с типами <T<MIN, T2...>, T<MIN + 1, T2...>, ...>.
        // По каждому номеру I от MIN до MAX хранится map[I] = T<I, T2...>.
        // Например, имеется тип Type и числа MIN = 3, MAX = 5.
        // Тип данных контейнера
        //      variant<Type<3>, Type<4>, Type<5>>
        // Элементы контейнера
        //      map[3] = Type<3>
        //      map[4] = Type<4>
        //      map[5] = Type<5>
        //

        // Объекты поддерживаются только у одного измерения. При загрузке
        // объекта одного измерения все объекты других измерений удаляются.
        std::unordered_map<int, SequenceVariant1<MIN, MAX, ObjectStorageSpace, StorageMeshFloatingPoint>> m_objects;

        void clear_all_data()
        {
                for (auto& p : m_objects)
                {
                        std::visit([&](auto& v) { v.clear_all_data(); }, p.second);
                }
        }

        void set_object_size_and_position(double size, const vec3& position) override
        {
                for (auto& p : m_objects)
                {
                        std::visit([&](auto& v) { v.set_object_size_and_position(size, position); }, p.second);
                }
        }

        void check_dimension(int dimension)
        {
                if (dimension < MIN || dimension > MAX)
                {
                        error("Error repository object dimension " + to_string(dimension) +
                              ", min = " + to_string(MIN) + ", max = " + to_string(MAX));
                }
        }

        std::vector<RepositoryObjects> repository_point_object_names() const override
        {
                std::vector<RepositoryObjects> names;

                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) { names.emplace_back(p.first, v.repository_point_object_names()); },
                                p.second);
                }

                return names;
        }

        bool manifold_constructor_exists() const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.manifold_constructor_exists() ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many manifold constructors " + to_string(count));
                }
                return count > 0;
        }

        bool object_exists(ObjectId id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.object_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many objects " + to_string(count));
                }
                return count > 0;
        }

        bool mesh_exists(ObjectId id) const override
        {
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit([&](const auto& v) { count += v.mesh_exists(id) ? 1 : 0; }, p.second);
                }
                if (count > 1)
                {
                        error("Too many meshes " + to_string(count));
                }
                return count > 0;
        }

        ObjectVariant object(ObjectId id) const override
        {
                if (!object_exists(id))
                {
                        error("No object");
                }
                std::optional<ObjectVariant> object_opt;
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.object_exists(id))
                                        {
                                                if (!object_opt)
                                                {
                                                        auto ptr = v.object(id);
                                                        if (!ptr)
                                                        {
                                                                error("Null object pointer");
                                                        }
                                                        auto m = v.object_matrix();
                                                        object_opt = Object(std::move(ptr), m);
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
                ASSERT(object_opt);
                return *object_opt;
        }

        MeshVariant mesh(ObjectId id) const override
        {
                if (!mesh_exists(id))
                {
                        error("No mesh");
                }
                std::optional<MeshVariant> mesh_opt;
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.mesh_exists(id))
                                        {
                                                if (!mesh_opt)
                                                {
                                                        auto ptr = v.mesh(id);
                                                        if (!ptr)
                                                        {
                                                                error("Null mesh pointer");
                                                        }
                                                        mesh_opt = std::move(ptr);
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
                ASSERT(mesh_opt);
                return *mesh_opt;
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
                        std::visit(
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

        void load_from_file(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double rho,
                double alpha) override
        {
                int dimension = mesh::file_dimension(file_name);

                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() { clear_all_data(); };
                std::visit(
                        [&](auto& v) {
                                v.load_from_file(objects, progress_list, file_name, rho, alpha, clear_function);
                        },
                        repository);
        }

        void load_from_repository(
                const std::unordered_set<ObjectId>& objects,
                ProgressRatioList* progress_list,
                int dimension,
                const std::string& object_name,
                double rho,
                double alpha,
                int point_count) override
        {
                check_dimension(dimension);

                auto& repository = m_objects.at(dimension);

                auto clear_function = [&]() { clear_all_data(); };
                std::visit(
                        [&](auto& v) {
                                v.load_from_repository(
                                        objects, progress_list, object_name, rho, alpha, point_count, clear_function);
                        },
                        repository);
        }

        void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const override
        {
                if (!object_exists(id))
                {
                        error("No object");
                }
                int count = 0;
                for (const auto& p : m_objects)
                {
                        std::visit(
                                [&](const auto& v) {
                                        if (v.object_exists(id))
                                        {
                                                if (count == 0)
                                                {
                                                        v.save(id, file_name, name);
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

        std::vector<FileFormat> formats_for_save(unsigned dimension) const override
        {
                std::vector<FileFormat> v(1);

                v[0].name = "OBJ Files";
                v[0].extensions = {mesh::obj_file_extension(dimension)};

                return v;
        }

        std::vector<FileFormat> formats_for_load() const override
        {
                std::set<unsigned> dimensions;
                for (int n = MIN; n <= MAX; ++n)
                {
                        dimensions.insert(n);
                }

                std::vector<FileFormat> v(1);

                v[0].name = "All Supported Formats";
                for (std::string& s : mesh::obj_file_supported_extensions(dimensions))
                {
                        v[0].extensions.push_back(std::move(s));
                }
                for (std::string& s : mesh::txt_file_supported_extensions(dimensions))
                {
                        v[0].extensions.push_back(std::move(s));
                }

                return v;
        }

        template <size_t... I>
        void init_map(
                int mesh_threads,
                const ObjectStorageEvents& event_emitter,
                const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler,
                std::integer_sequence<size_t, I...>&&)
        {
                static_assert(((I >= 0 && I < sizeof...(I)) && ...));
                static_assert(MIN + sizeof...(I) == MAX + 1);

                (m_objects.try_emplace(
                         MIN + I, std::in_place_type_t<ObjectStorageSpace<MIN + I, StorageMeshFloatingPoint>>(),
                         mesh_threads, event_emitter, exception_handler),
                 ...);

                ASSERT((m_objects.count(MIN + I) == 1) && ...);
                ASSERT(m_objects.size() == COUNT);
        }

public:
        Impl(int mesh_threads,
             const ObjectStorageEvents& event_emitter,
             const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
        {
                init_map(mesh_threads, event_emitter, exception_handler, std::make_integer_sequence<size_t, COUNT>());
        }
};
}

std::unique_ptr<ObjectStorage> create_object_storage(
        int mesh_threads,
        const ObjectStorageEvents& event_emitter,
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
{
        return std::make_unique<Impl>(mesh_threads, event_emitter, exception_handler);
}
