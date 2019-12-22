/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "query.h"

#include "com/error.h"
#include "com/print.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/functions.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace
{
void get_errors()
{
        while (glGetError() != GL_NO_ERROR)
        {
        }
}

GLint get_integer(GLenum pname)
{
        get_errors();

        GLint data;

        glGetIntegerv(pname, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("GetIntegerv error " + std::to_string(e));
        }

        return data;
}

#if 0
GLint get_integer_i(GLenum target, GLuint index)
{
        get_errors();

        GLint data;

        glGetIntegeri_v(target, index, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetIntegeri_v error " + std::to_string(e));
        }

        return data;
}
#endif

GLint64 get_integer_64(GLenum pname)
{
        get_errors();

        GLint64 data;

        glGetInteger64v(pname, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("GetInteger64v error " + std::to_string(e));
        }

        return data;
}

GLint64 get_integer_i_64(GLenum target, GLuint index)
{
        get_errors();

        GLint64 data;

        glGetInteger64i_v(target, index, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetInteger64i_v error " + std::to_string(e));
        }

        return data;
}

const char* get_string(GLenum name)
{
        get_errors();

        const GLubyte* data = glGetString(name);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetString error " + std::to_string(e));
        }

        return reinterpret_cast<const char*>(data);
}

const char* get_string_i(GLenum name, GLuint index)
{
        get_errors();

        const GLubyte* data = glGetStringi(name, index);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetStringi error " + std::to_string(e));
        }

        return reinterpret_cast<const char*>(data);
}

GLint get_named_framebuffer_attachment_parameter(GLuint framebuffer, GLenum attachment, GLenum pname)
{
        get_errors();

        GLint params;

        glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, &params);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetNamedFramebufferAttachmentParameteriv error " + std::to_string(e));
        }

        return params;
}

GLint get_named_framebuffer_parameter(GLuint framebuffer, GLenum pname)
{
        get_errors();

        GLint param;

        glGetNamedFramebufferParameteriv(framebuffer, pname, &param);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetNamedFramebufferParameteriv error " + std::to_string(e));
        }

        return param;
}
}

namespace opengl
{
void check_context(int major, int minor, const std::vector<std::string>& extensions)
{
        GLint mj = get_integer(GL_MAJOR_VERSION);
        GLint mn = get_integer(GL_MINOR_VERSION);
        if (mj < major || (mj == major && mn < minor))
        {
                error("OpenGL " + std::to_string(major) + "." + std::to_string(minor) + " is not supported. " + "Supported " +
                      std::to_string(mj) + "." + std::to_string(mn) + ".");
        }

        GLint m = get_integer(GL_CONTEXT_PROFILE_MASK);
        if (!(m & GL_CONTEXT_CORE_PROFILE_BIT) || (m & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
        {
                error("Not OpenGL Core Profile");
        }

        GLint num_ext = get_integer(GL_NUM_EXTENSIONS);
        std::vector<std::string> supported_extensions(num_ext);
        for (int i = 0; i < num_ext; ++i)
        {
                supported_extensions[i] = get_string_i(GL_EXTENSIONS, i);
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

void check_sizes(int sample_count, int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits, int alpha_bits)
{
        GLint p;

        p = framebuffer_samples();
        if (p < sample_count)
        {
                error("Context framebuffer samples " + std::to_string(p) + ". Required " + std::to_string(sample_count) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE);
        if (p < red_bits)
        {
                error("Context red bits " + std::to_string(p) + ". Required " + std::to_string(red_bits) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE);
        if (p < green_bits)
        {
                error("Context green bits " + std::to_string(p) + ". Required " + std::to_string(green_bits) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE);
        if (p < blue_bits)
        {
                error("Context blue bits " + std::to_string(p) + ". Required " + std::to_string(blue_bits) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE);
        if (p < alpha_bits)
        {
                error("Context alpha bits " + std::to_string(p) + ". Required " + std::to_string(alpha_bits) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE);
        if (p < depth_bits)
        {
                error("Context depth bits " + std::to_string(p) + ". Required " + std::to_string(depth_bits) + ".");
        }

        p = get_named_framebuffer_attachment_parameter(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE);
        if (p < stencil_bits)
        {
                error("Context stencil bits " + std::to_string(p) + ". Required " + std::to_string(stencil_bits) + ".");
        }
}

bool framebuffer_srgb()
{
        // GLint t = get_named_framebuffer_attachment_parameter(GL_DRAW_FRAMEBUFFER, GL_FRONT,
        // GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING);
        GLint t = get_named_framebuffer_attachment_parameter(0, GL_BACK, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING);

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

int framebuffer_samples()
{
        return get_named_framebuffer_parameter(0, GL_SAMPLES);
}

long long max_variable_group_size_x()
{
        return get_integer_i_64(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 0);
}

long long max_variable_group_size_y()
{
        return get_integer_i_64(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 1);
}

long long max_variable_group_size_z()
{
        return get_integer_i_64(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 2);
}

long long max_variable_group_invocations()
{
        return get_integer_64(GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB);
}

long long max_fixed_group_size_x()
{
        return get_integer_i_64(GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB, 0);
}

long long max_fixed_group_size_y()
{
        return get_integer_i_64(GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB, 1);
}

long long max_fixed_group_size_z()
{
        return get_integer_i_64(GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB, 2);
}

long long max_fixed_group_invocations()
{
        return get_integer_64(GL_MAX_COMPUTE_FIXED_GROUP_INVOCATIONS_ARB);
}

long long max_work_group_count_x()
{
        return get_integer_i_64(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0);
}

long long max_work_group_count_y()
{
        return get_integer_i_64(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1);
}

long long max_work_group_count_z()
{
        return get_integer_i_64(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2);
}

long long max_compute_shared_memory()
{
        return get_integer_64(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
}

long long max_texture_size()
{
        return get_integer_64(GL_MAX_TEXTURE_SIZE);
}

long long max_texture_buffer_size()
{
        return get_integer_64(GL_MAX_TEXTURE_BUFFER_SIZE);
}

long long max_shader_storage_block_size()
{
        return get_integer_64(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
}

const char* version()
{
        return get_string(GL_VERSION);
}

const char* vendor()
{
        return get_string(GL_VENDOR);
}

const char* renderer()
{
        return get_string(GL_RENDERER);
}

std::vector<const char*> context_flags()
{
        std::vector<const char*> r;

        GLint flags = get_integer(GL_CONTEXT_FLAGS);

        if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
        {
                r.push_back("CONTEXT_FLAG_FORWARD_COMPATIBLE");
        }

        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
                r.push_back("CONTEXT_FLAG_DEBUG");
        }

        if (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
        {
                r.push_back("CONTEXT_FLAG_ROBUST_ACCESS");
        }

        return r;
}
}

#endif
