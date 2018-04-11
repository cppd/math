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

#include "event_emitter.h"
#include "paintings.h"

#include "progress/progress_list.h"
#include "show/show.h"

#include <memory>
#include <string>
#include <vector>

struct MainObjects
{
        virtual ~MainObjects() = default;

        virtual std::vector<std::tuple<int, std::vector<std::string>>> repository_point_object_names() const = 0;

        virtual void set_show(IShow* show) = 0;

        virtual bool manifold_constructor_exists() const = 0;
        virtual bool object_exists(int id) const = 0;
        virtual bool mesh_exists(int id) const = 0;

        virtual std::string obj_extension(unsigned dimension) const = 0;
        virtual std::vector<std::string> obj_extensions() const = 0;
        virtual std::vector<std::string> txt_extensions() const = 0;

        virtual void compute_bound_cocone(ProgressRatioList* progress_list, double rho, double alpha) = 0;
        virtual void load_from_file(ProgressRatioList* progress_list, const std::string& file_name, double rho, double alpha) = 0;
        virtual void load_from_repository(ProgressRatioList* progress_list, const std::tuple<int, std::string>& object,
                                          double rho, double alpha, int point_count) = 0;
        virtual void save_to_file(int id, const std::string& file_name, const std::string& name) const = 0;
        virtual void paint(int id, const PaintingInformation3d& info_3d, const PaintingInformationNd& info_nd,
                           const PaintingInformationAll& info_all) const = 0;
};

std::unique_ptr<MainObjects> create_main_objects(int mesh_threads, const WindowEventEmitter& emitter);
