/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/numerical/vector.h>

#include <optional>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T>
struct TrueData final
{
        Vector<N, T> position;
        T speed;
        T angle;
        T angle_r;
};

template <std::size_t N, typename T>
struct Measurement final
{
        TrueData<N, T> true_data;

        T time;

        std::optional<Vector<N, T>> acceleration;
        T acceleration_variance;

        std::optional<T> direction;
        T direction_variance;

        std::optional<Vector<N, T>> position;
        T position_variance;

        std::optional<T> speed;
        T speed_variance;
};

template <std::size_t N, typename T>
class Track final
{
        class Iter final
        {
                typename std::vector<Measurement<N, T>>::const_iterator iter_;

        public:
                explicit Iter(typename std::vector<Measurement<N, T>>::const_iterator iter)
                        : iter_(std::move(iter))
                {
                }

                Iter& operator++()
                {
                        ++iter_;
                        return *this;
                }

                [[nodiscard]] bool operator==(const Iter& a) const
                {
                        return iter_ == a.iter_;
                }

                [[nodiscard]] const Measurement<N, T>& operator*() const
                {
                        return *iter_;
                }
        };

        std::vector<Measurement<N, T>> measurements_;
        std::string annotation_;

public:
        Track(std::vector<Measurement<N, T>> measurements, std::string annotation)
                : measurements_(std::move(measurements)),
                  annotation_(std::move(annotation))
        {
        }

        [[nodiscard]] Iter begin() const
        {
                return Iter(measurements_.cbegin());
        }

        [[nodiscard]] Iter end() const
        {
                return Iter(measurements_.cend());
        }

        [[nodiscard]] const std::string& annotation() const
        {
                return annotation_;
        }
};

template <std::size_t N, typename T>
[[nodiscard]] Track<N, T> track();
}
