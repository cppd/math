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

#include "paintings.h"

#include "progress/progress_list.h"
#include "show/show.h"

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

enum class ObjectId
{
        Model,
        ModelMst,
        ModelConvexHull,
        Cocone,
        CoconeConvexHull,
        BoundCocone,
        BoundCoconeConvexHull
};

int object_id_to_int(ObjectId id);
ObjectId int_to_object_id(int id);

class IObjectsCallback
{
protected:
        virtual ~IObjectsCallback() = default;

public:
        virtual void file_loaded(const std::string& msg, unsigned dimension, const std::unordered_set<ObjectId>& objects) const
                noexcept = 0;
        virtual void bound_cocone_loaded(double rho, double alpha) const noexcept = 0;
        virtual void message_warning(const std::string& msg) const noexcept = 0;
};

struct MainObjects
{
        virtual ~MainObjects() = default;

        virtual std::vector<std::tuple<int, std::vector<std::string>>> repository_point_object_names() const = 0;

        virtual void set_show(IShow* show) = 0;

        virtual bool manifold_constructor_exists() const = 0;
        virtual bool object_exists(ObjectId id) const = 0;
        virtual bool mesh_exists(ObjectId id) const = 0;

        virtual std::string obj_extension(unsigned dimension) const = 0;
        virtual std::vector<std::string> obj_extensions() const = 0;
        virtual std::vector<std::string> txt_extensions() const = 0;

        virtual void compute_bound_cocone(const std::unordered_set<ObjectId>& objects, ProgressRatioList* progress_list,
                                          double rho, double alpha) = 0;

        virtual void load_from_file(const std::unordered_set<ObjectId>& objects, ProgressRatioList* progress_list,
                                    const std::string& file_name, double rho, double alpha) = 0;
        virtual void load_from_repository(const std::unordered_set<ObjectId>& objects, ProgressRatioList* progress_list,
                                          const std::tuple<int, std::string>& object, double rho, double alpha,
                                          int point_count) = 0;

        virtual void save_to_file(ObjectId id, const std::string& file_name, const std::string& name) const = 0;

        virtual void paint(ObjectId id, const PaintingInformation3d& info_3d, const PaintingInformationNd& info_nd,
                           const PaintingInformationAll& info_all) const = 0;
};

std::unique_ptr<MainObjects> create_main_objects(
        int mesh_threads, const IObjectsCallback& event_emitter,
        std::function<void(const std::exception_ptr& ptr, const std::string& msg)> exception_handler);
