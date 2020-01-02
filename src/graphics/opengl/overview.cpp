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

#if defined(OPENGL_FOUND)

#include "overview.h"

#include "query.h"

#include <sstream>

namespace opengl
{
std::string overview()
{
        std::ostringstream oss;

        oss << "GL_VERSION: " << version() << "\n";
        oss << "GL_VENDOR: " << vendor() << "\n";
        oss << "GL_RENDERER: " << renderer() << "\n";

        for (const auto& s : context_flags())
        {
                oss << s << "\n";
        }

        oss << "framebuffer: " << (framebuffer_srgb() ? "sRGB" : "RGB") << "\n";
        oss << "max variable group size x: " << max_variable_group_size_x() << "\n";
        oss << "max variable group size y: " << max_variable_group_size_y() << "\n";
        oss << "max variable group size z: " << max_variable_group_size_z() << "\n";
        oss << "max variable group invocations: " << max_variable_group_invocations() << "\n";
        oss << "max fixed group size x: " << max_fixed_group_size_x() << "\n";
        oss << "max fixed group size y: " << max_fixed_group_size_y() << "\n";
        oss << "max fixed group size z: " << max_fixed_group_size_z() << "\n";
        oss << "max fixed group invocations: " << max_fixed_group_invocations() << "\n";
        oss << "max work group count x: " << max_work_group_count_x() << "\n";
        oss << "max work group count y: " << max_work_group_count_y() << "\n";
        oss << "max work group count z: " << max_work_group_count_z() << "\n";
        oss << "max compute shared memory: " << max_compute_shared_memory() << "\n";
        oss << "max texture size: " << max_texture_size() << "\n";
        oss << "max texture buffer size: " << max_texture_buffer_size() << "\n";
        oss << "max shader storage block size: " << max_shader_storage_block_size() << "\n";
        oss << "samples: " << framebuffer_samples() << "\n";

        return oss.str();
}
}

#endif
