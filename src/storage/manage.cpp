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

#include "manage.h"

#include "processor.h"
#include "storage.h"

#include <src/com/error.h>
#include <src/model/mesh_utility.h>

#include <tuple>

namespace
{
constexpr int MIN = STORAGE_MIN_DIMENSIONS;
constexpr int MAX = STORAGE_MAX_DIMENSIONS;
constexpr size_t COUNT = MAX - MIN + 1;

static_assert(MIN >= 3 && MIN <= MAX);
static_assert(std::is_floating_point_v<StorageMeshFloatingPoint>);

[[noreturn]] void error_dimension_not_supported(unsigned dimension)
{
        error("Dimension " + to_string(dimension) + " is not supported, min dimension = " + to_string(MIN) +
              ", max dimension = " + to_string(MAX));
}

class Impl final : public StorageManage
{
        template <size_t DIMENSION>
        struct Data final
        {
                static constexpr size_t N = DIMENSION;

                const std::unique_ptr<const ObjectRepository<N>> repository;
                ObjectStorage<N, StorageMeshFloatingPoint> storage;

                explicit Data(const StorageEvents& storage_events)
                        : repository(create_object_repository<N>()), storage(storage_events)
                {
                }
        };

        // std::tuple<Data<MIN>, ..., Data<MAX>>.
        SequenceType1<std::tuple, MIN, MAX, Data> m_data;

        void clear_all_data()
        {
                std::apply([](auto&... v) { (v.storage.clear(), ...); }, m_data);
        }

        std::vector<RepositoryObjects> repository_point_object_names() const override
        {
                std::vector<RepositoryObjects> names;

                std::apply(
                        [&](const auto&... v) { (names.emplace_back(v.N, v.repository->point_object_names()), ...); },
                        m_data);

                return names;
        }

        std::optional<ObjectVariant> object(ObjectId id) const override
        {
                std::optional<ObjectVariant> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.storage.object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }() ||
                                 ...);
                        },
                        m_data);

                return opt;
        }

        std::optional<MeshVariant> mesh(ObjectId id) const override
        {
                std::optional<MeshVariant> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.storage.mesh(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }() ||
                                 ...);
                        },
                        m_data);

                return opt;
        }

        void compute_bound_cocone(
                ProgressRatioList* progress_list,
                ObjectId id,
                double rho,
                double alpha,
                int mesh_threads) override
        {
                bool found = std::apply(
                        [&](auto&... v) {
                                return ([&]() {
                                        if (!v.storage.object(id))
                                        {
                                                return false;
                                        }

                                        processor::compute_bound_cocone(
                                                progress_list, &v.storage, id, rho, alpha, mesh_threads);

                                        return true;
                                }() || ...);
                        },
                        m_data);

                if (!found)
                {
                        error("No object found");
                }
        }

        void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const override
        {
                bool found = std::apply(
                        [&](const auto&... v) {
                                return ([&]() {
                                        if (!v.storage.object(id))
                                        {
                                                return false;
                                        }

                                        processor::save(v.storage, id, file_name, name);

                                        return true;
                                }() || ...);
                        },
                        m_data);

                if (!found)
                {
                        error("No object found");
                }
        }

        void load_from_file(
                bool build_convex_hull,
                bool build_cocone,
                bool build_bound_cocone,
                bool build_mst,
                ProgressRatioList* progress_list,
                const std::string& file_name,
                double object_size,
                const vec3& object_position,
                double rho,
                double alpha,
                int mesh_threads,
                const std::function<void(size_t dimension)>& load_event) override
        {
                unsigned dimension = mesh::file_dimension(file_name);

                bool found = std::apply(
                        [&](auto&... v) {
                                return ([&]() {
                                        constexpr unsigned N = std::remove_reference_t<decltype(v)>::N;

                                        if (N != dimension)
                                        {
                                                return false;
                                        }

                                        auto mesh = processor::load_from_file<N>(progress_list, file_name);

                                        clear_all_data();
                                        load_event(dimension);

                                        processor::compute(
                                                progress_list, &v.storage, build_convex_hull, build_cocone,
                                                build_bound_cocone, build_mst, std::move(mesh), object_size,
                                                object_position, rho, alpha, mesh_threads);

                                        return true;
                                }() || ...);
                        },
                        m_data);

                if (!found)
                {
                        error_dimension_not_supported(dimension);
                }
        }

        void load_from_repository(
                bool build_convex_hull,
                bool build_cocone,
                bool build_bound_cocone,
                bool build_mst,
                ProgressRatioList* progress_list,
                int dimension,
                const std::string& object_name,
                double object_size,
                const vec3& object_position,
                double rho,
                double alpha,
                int mesh_threads,
                int point_count,
                const std::function<void()>& load_event) override
        {
                bool found = std::apply(
                        [&](auto&... v) {
                                return ([&]() {
                                        constexpr unsigned N = std::remove_reference_t<decltype(v)>::N;

                                        if (N != dimension)
                                        {
                                                return false;
                                        }

                                        auto mesh = processor::load_from_repository(
                                                progress_list, *v.repository, object_name, point_count);

                                        clear_all_data();
                                        load_event();

                                        processor::compute(
                                                progress_list, &v.storage, build_convex_hull, build_cocone,
                                                build_bound_cocone, build_mst, std::move(mesh), object_size,
                                                object_position, rho, alpha, mesh_threads);

                                        return true;
                                }() || ...);
                        },
                        m_data);

                if (!found)
                {
                        error_dimension_not_supported(dimension);
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
        Impl(const StorageEvents& storage_events, std::integer_sequence<size_t, I...>&&)
                : m_data((static_cast<void>(I), storage_events)...)
        {
        }

public:
        explicit Impl(const StorageEvents& storage_events)
                : Impl(storage_events, std::make_integer_sequence<size_t, COUNT>())
        {
        }
};
}

std::unique_ptr<StorageManage> create_storage_manage(const StorageEvents& storage_events)
{
        return std::make_unique<Impl>(storage_events);
}
