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

#include "write.h"

#include <src/settings/name.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace
{
std::mutex g_lock;

#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif
const std::chrono::steady_clock::time_point START_TIME = std::chrono::steady_clock::now();
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

class Log final
{
        static constexpr unsigned MAX_THREADS = 1'000'000;
        static constexpr unsigned THREADS_WIDTH = 6;

        std::unordered_map<std::thread::id, unsigned> m_map;
        unsigned m_width = THREADS_WIDTH;
        std::ostringstream m_oss_line_beginning;
        std::string m_line_beginning;
        std::string m_result;

        std::ofstream m_file;

        void write_thread_id(const std::thread::id& thread_id) noexcept
        {
                auto iter = m_map.find(thread_id);
                if (iter == m_map.cend())
                {
                        if (m_map.size() < MAX_THREADS)
                        {
                                iter = m_map.emplace(thread_id, m_map.size()).first;
                        }
                        else
                        {
                                m_width = 18;
                                m_oss_line_beginning << "0x" << std::hex << std::setw(16) << thread_id;
                                return;
                        }
                }
                m_oss_line_beginning << std::setw(m_width) << iter->second;
        }

        std::string format(const std::string_view& text, const std::string_view& description, double time) noexcept
        {
                m_oss_line_beginning.str(std::string());
                m_oss_line_beginning << "[" << std::setw(11) << time << "][";
                write_thread_id(std::this_thread::get_id());
                m_oss_line_beginning << "]";

                m_line_beginning = m_oss_line_beginning.str();
                if (description.empty())
                {
                        m_line_beginning += ": ";
                }
                else
                {
                        m_line_beginning += "(";
                        for (char c : description)
                        {
                                m_line_beginning += std::isalpha(static_cast<unsigned char>(c)) ? c : ' ';
                        }
                        m_line_beginning += "): ";
                }

                m_result.clear();
                m_result.reserve(m_line_beginning.size() + text.size() + 1);
                m_result = m_line_beginning;
                for (char c : text)
                {
                        m_result += c;
                        if (c == '\n')
                        {
                                m_result += m_line_beginning;
                        }
                }
                m_result += '\n';
                return m_result;
        }

        void write(const std::string_view& text) noexcept
        {
                std::cerr << text;
                m_file << text;
        }

public:
        Log()
        {
                m_oss_line_beginning << std::fixed;
                m_oss_line_beginning << std::setfill('0');
                m_oss_line_beginning << std::right;
                m_oss_line_beginning << std::setprecision(6);

                std::string name = std::string(settings::APPLICATION_NAME) + " log.txt";
                std::filesystem::path path = std::filesystem::temp_directory_path() / name;
                m_file = std::ofstream(path);
                m_file << std::unitbuf;
        }

        std::string write(const std::string_view& text, const std::string_view& description, double time) noexcept
        {
                std::string result = format(text, description, time);
                write(result);
                result.pop_back();
                return result;
        }
};

std::string write(const std::string_view& text, const std::string_view& description) noexcept
{
        const double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - START_TIME).count();
        static Log log;
        return log.write(text, description, time);
}
}

std::string write_log(const std::string_view& text, const std::string_view& description) noexcept
{
        std::lock_guard lg(g_lock);
        return write(text, description);
}

void write_log_fatal_error_and_exit(const char* text) noexcept
{
        std::lock_guard lg(g_lock);
        write(text, "fatal error");
        std::_Exit(EXIT_FAILURE);
}
