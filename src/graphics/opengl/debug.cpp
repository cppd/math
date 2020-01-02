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

#include "debug.h"

#include "capabilities.h"
#include "functions.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"

#include <atomic>
#include <sstream>

namespace
{
void APIENTRY debug_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message,
                         const void* /*userParam*/)
{
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        {
                return;
        }

        const char* source_str;
        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
                source_str = "API";
                break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
                source_str = "Shader Compiler";
                break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                source_str = "Window System";
                break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
                source_str = "Third Party";
                break;
        case GL_DEBUG_SOURCE_APPLICATION:
                source_str = "Application";
                break;
        case GL_DEBUG_SOURCE_OTHER:
                source_str = "Other";
                break;
        default:
                source_str = "Unknown";
        }

        const char* type_str;
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
                type_str = "Error";
                break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                type_str = "Deprecated Behavior";
                break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                type_str = "Undefined Behavior";
                break;
        case GL_DEBUG_TYPE_PERFORMANCE:
                type_str = "Performance";
                break;
        case GL_DEBUG_TYPE_PORTABILITY:
                type_str = "Portability";
                break;
        case GL_DEBUG_TYPE_MARKER:
                type_str = "Marker";
                break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
                type_str = "Push Group";
                break;
        case GL_DEBUG_TYPE_POP_GROUP:
                type_str = "Pop Group";
                break;
        case GL_DEBUG_TYPE_OTHER:
                type_str = "Other";
                break;
        default:
                type_str = "Unknown";
        }

        const char* severity_str;
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
                severity_str = "High";
                break;
        case GL_DEBUG_SEVERITY_MEDIUM:
                severity_str = "Medium";
                break;
        case GL_DEBUG_SEVERITY_LOW:
                severity_str = "Low";
                break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
                severity_str = "Notification";
                break;
        default:
                severity_str = "Unknown";
        }

        ASSERT(source_str && type_str && severity_str && message);

        std::ostringstream oss;
        oss << "Debug Message (";
        oss << "id = " << id;
        oss << "; ";
        oss << "source = " << source_str;
        oss << "; ";
        oss << "type = " << type_str;
        oss << "; ";
        oss << "severity = " << severity_str;
        oss << "): ";
        oss << message;

        LOG(oss.str());
}

//

class DebugCounter
{
        static std::atomic_int m_counter;

public:
        DebugCounter()
        {
                if (++m_counter != 1)
                {
                        --m_counter;
                        error("Too many debug message classes");
                }
        }
        ~DebugCounter()
        {
                --m_counter;
        }
        DebugCounter(const DebugCounter&) = delete;
        DebugCounter& operator=(const DebugCounter&) = delete;
        DebugCounter(DebugCounter&&) = delete;
        DebugCounter& operator=(DebugCounter&&) = delete;
};
std::atomic_int DebugCounter::m_counter = 0;

class Impl final : public opengl::DebugMessage
{
        DebugCounter m_debug_counter;
        opengl::GLEnableAndRestore<GL_DEBUG_OUTPUT> m_debug_enable;

public:
        Impl()
        {
                glDebugMessageCallback(debug_proc, nullptr);
        }
        ~Impl() override
        {
                glDebugMessageCallback(nullptr, nullptr);
        }
        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

namespace opengl
{
std::unique_ptr<DebugMessage> create_debug_message()
{
        return std::make_unique<Impl>();
}
}

#endif
