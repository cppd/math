/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/log.h>

#include <string>

namespace ns::gpu::renderer
{
class TransparencyMessage final
{
        static long long to_mb(const long long value)
        {
                return value >> 20;
        }

        const std::string node_buffer_max_size_mb_;

        long long previous_required_node_memory_ = -1;
        long long previous_overload_count_ = -1;

        void process_required_node_memory(const long long required_node_memory)
        {
                if (required_node_memory < 0)
                {
                        if (previous_required_node_memory_ >= 0)
                        {
                                LOG("Transparency memory: OK");
                        }
                }
                else
                {
                        if (previous_required_node_memory_ != required_node_memory)
                        {
                                std::string s;
                                s += "Transparency memory: required ";
                                s += std::to_string(to_mb(required_node_memory));
                                s += " MiB, limit ";
                                s += node_buffer_max_size_mb_;
                                s += " MiB.";
                                LOG(s);
                        }
                }
                previous_required_node_memory_ = required_node_memory;
        }

        void process_overload_count(const long long overload_count)
        {
                if (overload_count < 0)
                {
                        if (previous_overload_count_ >= 0)
                        {
                                LOG("Transparency overload: OK");
                        }
                }
                else
                {
                        if (previous_overload_count_ != overload_count)
                        {
                                std::string s;
                                s += "Transparency overload: ";
                                s += std::to_string(overload_count);
                                s += " samples.";
                                LOG(s);
                        }
                }
                previous_overload_count_ = overload_count;
        }

public:
        explicit TransparencyMessage(const long long node_buffer_max_size)
                : node_buffer_max_size_mb_(std::to_string(to_mb(node_buffer_max_size)))
        {
        }

        void process(const long long required_node_memory, const long long overload_count)
        {
                process_required_node_memory(required_node_memory);
                process_overload_count(overload_count);
        }
};
}
