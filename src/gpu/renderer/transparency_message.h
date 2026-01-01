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

#include <src/com/log.h>

#include <optional>
#include <string>

namespace ns::gpu::renderer
{
class TransparencyMessage final
{
        static unsigned long long to_mb(const unsigned long long value)
        {
                return value >> 20;
        }

        const std::string node_buffer_max_size_mb_;

        std::optional<unsigned long long> previous_required_node_memory_;
        std::optional<unsigned long long> previous_overload_count_;

        void process_required_node_memory(const std::optional<unsigned long long> required_node_memory)
        {
                if (!required_node_memory)
                {
                        if (previous_required_node_memory_)
                        {
                                LOG("Transparency memory: OK");
                        }
                }
                else
                {
                        if (previous_required_node_memory_ != *required_node_memory)
                        {
                                std::string s;
                                s += "Transparency memory: required ";
                                s += std::to_string(to_mb(*required_node_memory));
                                s += " MiB, limit ";
                                s += node_buffer_max_size_mb_;
                                s += " MiB.";
                                LOG(s);
                        }
                }
                previous_required_node_memory_ = required_node_memory;
        }

        void process_overload_count(const std::optional<unsigned long long> overload_count)
        {
                if (!overload_count)
                {
                        if (previous_overload_count_)
                        {
                                LOG("Transparency overload: OK");
                        }
                }
                else
                {
                        if (previous_overload_count_ != *overload_count)
                        {
                                std::string s;
                                s += "Transparency overload: ";
                                s += std::to_string(*overload_count);
                                s += " samples.";
                                LOG(s);
                        }
                }
                previous_overload_count_ = overload_count;
        }

public:
        explicit TransparencyMessage(const unsigned long long node_buffer_max_size)
                : node_buffer_max_size_mb_(std::to_string(to_mb(node_buffer_max_size)))
        {
        }

        struct Data final
        {
                std::optional<unsigned long long> required_node_memory;
                std::optional<unsigned long long> overload_count;
        };

        void process(const Data& data)
        {
                process_required_node_memory(data.required_node_memory);
                process_overload_count(data.overload_count);
        }
};
}
