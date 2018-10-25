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

#include "query.h"

#include "com/error.h"
#include "graphics/opengl/functions/opengl_functions.h"
#include "graphics/opengl/objects.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace
{
GLint get_integer(GLenum pname)
{
        glGetError();

        GLint data;

        glGetIntegerv(pname, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("GetIntegerv error " + std::to_string(e));
        }

        return data;
}

GLint get_integer_i(GLenum target, GLuint index)
{
        glGetError();

        GLint data;

        glGetIntegeri_v(target, index, &data);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetIntegeri_v error " + std::to_string(e));
        }

        return data;
}

const GLubyte* get_string(GLenum name)
{
        glGetError();

        const GLubyte* data = glGetString(name);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetString error " + std::to_string(e));
        }

        return data;
}

const GLubyte* get_string_i(GLenum name, GLuint index)
{
        glGetError();

        const GLubyte* data = glGetStringi(name, index);

        GLenum e = glGetError();
        if (e != GL_NO_ERROR)
        {
                error("glGetStringi error " + std::to_string(e));
        }

        return data;
}

GLint get_named_framebuffer_attachment_parameter(GLuint framebuffer, GLenum attachment, GLenum pname)
{
        glGetError();

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
        glGetError();

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
std::string overview()
{
        std::ostringstream os;

        os << "GL_VERSION: " << get_string(GL_VERSION) << "\n";
        os << "GL_VENDOR: " << get_string(GL_VENDOR) << "\n";
        os << "GL_RENDERER: " << get_string(GL_RENDERER) << "\n";

        GLint flags = get_integer(GL_CONTEXT_FLAGS);
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

        os << "framebuffer: " << (framebuffer_srgb() ? "sRGB" : "RGB") << "\n";
        os << "max work group size x: " << max_work_group_size_x() << "\n";
        os << "max work group size y: " << max_work_group_size_y() << "\n";
        os << "max work group size z: " << max_work_group_size_z() << "\n";
        os << "max work group invocations: " << max_work_group_invocations() << "\n";
        os << "max work group count x: " << max_work_group_count_x() << "\n";
        os << "max work group count y: " << max_work_group_count_y() << "\n";
        os << "max work group count z: " << max_work_group_count_z() << "\n";
        os << "max compute shared memory: " << max_compute_shared_memory() << "\n";
        os << "max texture size: " << max_texture_size() << "\n";
        // os << "max texture buffer size: " << max_texture_buffer_size() << "\n";
        os << "max shader storage block size: " << max_shader_storage_block_size() << "\n";
        os << "samples: " << framebuffer_samples() << "\n";

        return os.str();
}

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
                supported_extensions[i] = reinterpret_cast<const char*>(get_string_i(GL_EXTENSIONS, i));
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

// framebuffer_srgb() возвращает неправильные значения.
// Поэтому нужно записать цвет в буфер и прочитать обратно.
bool current_buffer_is_srgb()
{
        constexpr float EPSILON = 0.01;

#if 1
        constexpr float RGB_COLOR = 0.5;
        constexpr float SRGB_COLOR = 0.73725;
#else
        constexpr float RGB_COLOR = 0.1;
        constexpr float SRGB_COLOR = 0.34902;
#endif

        float source_color[4] = {RGB_COLOR, RGB_COLOR, RGB_COLOR, 1};
        glClearBufferfv(GL_COLOR, 0, source_color);

        TextureRGBA32F pixel_texture(1, 1);
        pixel_texture.copy_texture_sub_image();

        std::array<GLfloat, 4> pixel{-1, -1, -1, -1};
        pixel_texture.get_texture_sub_image(0, 0, 1, 1, &pixel);

        const float buffer_color = pixel[0];

        if (!(std::abs(buffer_color - RGB_COLOR) < EPSILON || std::abs(buffer_color - SRGB_COLOR) < EPSILON))
        {
                error("Buffer color space detection failed. RGB color " + to_string(RGB_COLOR) + " from buffer is " +
                      to_string(buffer_color) + ". Expected either RGB color " + to_string(RGB_COLOR) + " or sRGB color " +
                      to_string(SRGB_COLOR) + ".");
        }

        return std::abs(buffer_color - SRGB_COLOR) < EPSILON;
}

int framebuffer_samples()
{
        return get_named_framebuffer_parameter(0, GL_SAMPLES);
}

int max_work_group_size_x()
{
        return get_integer_i(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 0);
}

int max_work_group_size_y()
{
        return get_integer_i(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 1);
}

int max_work_group_size_z()
{
        return get_integer_i(GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB, 2);
}

int max_work_group_invocations()
{
        return get_integer(GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB);
}

int max_work_group_count_x()
{
        return get_integer_i(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0);
}

int max_work_group_count_y()
{
        return get_integer_i(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1);
}

int max_work_group_count_z()
{
        return get_integer_i(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2);
}

int max_compute_shared_memory()
{
        return get_integer(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
}

int max_texture_size()
{
        return get_integer(GL_MAX_TEXTURE_SIZE);
}

int max_texture_buffer_size()
{
        return get_integer(GL_MAX_TEXTURE_BUFFER_SIZE);
}

int max_shader_storage_block_size()
{
        return get_integer(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
}
}
