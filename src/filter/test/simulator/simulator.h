/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ns::filter::test::simulator
{
template <std::size_t N, typename T>
class VarianceCorrection
{
public:
        virtual ~VarianceCorrection() = default;

        virtual void reset() = 0;
        virtual void correct(filters::Measurements<N, T>* m) = 0;
};

template <std::size_t N, typename T>
class Track final
{
        std::vector<filters::Measurements<N, T>> measurements_;
        std::unique_ptr<VarianceCorrection<N, T>> variance_correction_;
        std::string annotation_;

public:
        Track(std::vector<filters::Measurements<N, T>> measurements,
              std::unique_ptr<VarianceCorrection<N, T>>&& variance_correction,
              std::string annotation)
                : measurements_(std::move(measurements)),
                  variance_correction_(std::move(variance_correction)),
                  annotation_(std::move(annotation))
        {
                ASSERT(variance_correction_);
        }

        [[nodiscard]] const std::vector<filters::Measurements<N, T>>& measurements() const
        {
                return measurements_;
        }

        [[nodiscard]] VarianceCorrection<N, T>& variance_correction() const
        {
                return *variance_correction_;
        }

        [[nodiscard]] const std::string& annotation() const
        {
                return annotation_;
        }
};

template <std::size_t N, typename T>
[[nodiscard]] Track<N, T> track();
}
