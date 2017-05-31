/*
Copyright (C) 2017 Topological Manifold

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

#include "gl_query.h"

#include "com/error.h"
#include "gl_func/gl_functions.h"

#include <algorithm>
#include <iostream>
#include <sstream>

void print_settings()
{
        std::ostringstream os;

        os << "\n";

        os << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";
        os << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n";
        os << "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";

        int flags;

        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
        {
                os << "CONTEXT_FLAG_FORWARD_COMPATIBLE\n";
        }
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
                os << "CONTEXT_FLAG_DEBUG\n";
        }
        if (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
        {
                os << "CONTEXT_FLAG_ROBUST_ACCESS\n";
        }

        os << "framebuffer: " << (get_framebuffer_sRGB() ? "sRGB" : "RGB") << "\n";
        os << "max work group size x: " << get_max_work_group_size_x() << "\n";
        os << "max work group size y: " << get_max_work_group_size_y() << "\n";
        os << "max work group size z: " << get_max_work_group_size_z() << "\n";
        os << "max work group invocations: " << get_max_work_group_invocations() << "\n";
        os << "max work group count x: " << get_max_work_group_count_x() << "\n";
        os << "max work group count y: " << get_max_work_group_count_y() << "\n";
        os << "max work group count z: " << get_max_work_group_count_z() << "\n";
        os << "max compute shared memory: " << get_max_compute_shared_memory() << "\n";
        os << "max texture size: " << get_max_texture_size() << "\n";
        // os << "max texture buffer size: " << get_max_texture_buffer_size() << "\n";
        os << "max shader storage block size: " << get_max_shader_storage_block_size() << "\n";
        os << "samples: " << get_framebuffer_samples() << "\n";

        os << "\n";

        std::cout << os.str() << std::flush;
}

void check_context(int major, int minor, const std::vector<std::string>& extensions)
{
        GLint mj, mn;
        glGetIntegerv(GL_MAJOR_VERSION, &mj);
        glGetIntegerv(GL_MINOR_VERSION, &mn);
        if (mj < major || (mj == major && mn < minor))
        {
                error("OpenGL " + std::to_string(major) + "." + std::to_string(minor) + " is not supported. " + "Supported " +
                      std::to_string(mj) + "." + std::to_string(mn) + ".");
        }

        GLint m;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &m);
        if (!(m & GL_CONTEXT_CORE_PROFILE_BIT) || (m & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
        {
                error("Not OpenGL Core Profile");
        }

        GLint num_ext;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
        std::vector<std::string> supported_extensions(num_ext);
        for (int i = 0; i < num_ext; ++i)
        {
                supported_extensions[i] = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        }
        std::sort(supported_extensions.begin(), supported_extensions.end());

        for (const std::string& ext : extensions)
        {
                if (!std::binary_search(supported_extensions.begin(), supported_extensions.end(), ext))
                {
                        error("OpenGL extension " + ext + " is not supported");
                }
        }
}

void check_bit_sizes(int depthBits, int stencilBits, int antialiasing_level, int redBits, int greenBits, int blueBits,
                     int alphaBits)
{
        GLint p;

        p = get_framebuffer_samples();
        if (p < antialiasing_level)
        {
                error("Context framebuffer samples " + std::to_string(p) + ". Required " + std::to_string(antialiasing_level) +
                      ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &p);
        if (p < redBits)
        {
                error("Context red bits " + std::to_string(p) + ". Required " + std::to_string(redBits) + ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &p);
        if (p < greenBits)
        {
                error("Context green bits " + std::to_string(p) + ". Required " + std::to_string(greenBits) + ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &p);
        if (p < blueBits)
        {
                error("Context blue bits " + std::to_string(p) + ". Required " + std::to_string(blueBits) + ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &p);
        if (p < alphaBits)
        {
                error("Context alpha bits " + std::to_string(p) + ". Required " + std::to_string(alphaBits) + ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &p);
        if (p < depthBits)
        {
                error("Context depth bits " + std::to_string(p) + ". Required " + std::to_string(depthBits) + ".");
        }
        glGetNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &p);
        if (p < stencilBits)
        {
                error("Context stencil bits " + std::to_string(p) + ". Required " + std::to_string(stencilBits) + ".");
        }
}

bool get_framebuffer_sRGB()
{
        GLint t;

        // glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRONT,
        // GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &t);
        glGetNamedFramebufferAttachmentParameteriv(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &t);
        if (t == GL_SRGB)
        {
                return true;
        }
        else if (t == GL_LINEAR)
        {
                return false;
        }
        else
        {
                error("Error find FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING");
        }
}
int get_framebuffer_samples()
{
        GLint p;
        glGetNamedFramebufferParameteriv(0, GL_SAMPLES, &p);
        return p;
}
int get_max_work_group_size_x()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 0, &max_size);
        return max_size;
}
int get_max_work_group_size_y()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 1, &max_size);
        return max_size;
}
int get_max_work_group_size_z()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 2, &max_size);
        return max_size;
}
int get_max_work_group_invocations()
{
        GLint max_size;
        glGetIntegerv(GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB, &max_size);
        return max_size;
}
int get_max_work_group_count_x()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_size);
        return max_size;
}
int get_max_work_group_count_y()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_size);
        return max_size;
}
int get_max_work_group_count_z()
{
        GLint max_size;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_size);
        return max_size;
}
int get_max_compute_shared_memory()
{
        GLint max_size;
        glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &max_size);
        return max_size;
}
int get_max_texture_size()
{
        GLint max_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
        return max_size;
}
int get_max_texture_buffer_size()
{
        GLint max_size;
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_size);
        return max_size;
}
int get_max_shader_storage_block_size()
{
        GLint max_size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_size);
        return max_size;
}
