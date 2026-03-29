/*
 * This file is part of Borealis NWN Model Viewer.
 * Copyright (C) 2025-2026 Varenx
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Shader.hpp"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <spdlog/spdlog.h>
#include <vector>

namespace nwn
{

    // Get OpenGL functions
    static QOpenGLFunctions_3_3_Core* gl()
    {
        static QOpenGLFunctions_3_3_Core* funcs = nullptr;
        if (!funcs)
        {
            funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(
                QOpenGLContext::currentContext());
        }
        return funcs;
    }

    Shader::~Shader()
    {
        if (m_Program != 0)
        {
            gl()->glDeleteProgram(m_Program);
        }
    }

    Shader::Shader(Shader&& other) noexcept :
        m_Program(other.m_Program), m_UniformCache(std::move(other.m_UniformCache))
    {
        other.m_Program = 0;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            if (m_Program != 0)
            {
                gl()->glDeleteProgram(m_Program);
            }
            m_Program = other.m_Program;
            m_UniformCache = std::move(other.m_UniformCache);
            other.m_Program = 0;
        }
        return *this;
    }

    bool Shader::compile(std::string_view vertexSource, std::string_view fragmentSource)
    {
        uint32_t vertShader = 0, fragShader = 0;

        if (!compileShader(GL_VERTEX_SHADER, vertexSource, vertShader))
        {
            return false;
        }

        if (!compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragShader))
        {
            gl()->glDeleteShader(vertShader);
            return false;
        }

        if (!linkProgram(vertShader, fragShader))
        {
            gl()->glDeleteShader(vertShader);
            gl()->glDeleteShader(fragShader);
            return false;
        }

        // Shaders can be deleted after linking
        gl()->glDeleteShader(vertShader);
        gl()->glDeleteShader(fragShader);

        return true;
    }

    bool Shader::compileShader(uint32_t type, std::string_view source, uint32_t& shader)
    {
        shader = gl()->glCreateShader(type);

        const char* src = source.data();
        int len = static_cast<int>(source.size());
        gl()->glShaderSource(shader, 1, &src, &len);
        gl()->glCompileShader(shader);

        int success;
        gl()->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            gl()->glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            spdlog::error("Shader compilation failed: {}", log);
            gl()->glDeleteShader(shader);
            shader = 0;
            return false;
        }

        return true;
    }

    bool Shader::linkProgram(uint32_t vertShader, uint32_t fragShader)
    {
        m_Program = gl()->glCreateProgram();
        gl()->glAttachShader(m_Program, vertShader);
        gl()->glAttachShader(m_Program, fragShader);
        gl()->glLinkProgram(m_Program);

        int success;
        gl()->glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            gl()->glGetProgramInfoLog(m_Program, sizeof(log), nullptr, log);
            spdlog::error("Shader linking failed: {}", log);
            gl()->glDeleteProgram(m_Program);
            m_Program = 0;
            return false;
        }

        return true;
    }

    void Shader::use() const
    {
        gl()->glUseProgram(m_Program);
    }

    int32_t Shader::getUniformLocation(const char* name) const
    {
        auto it = m_UniformCache.find(name);
        if (it != m_UniformCache.end())
        {
            return it->second;
        }

        int32_t location = gl()->glGetUniformLocation(m_Program, name);
        m_UniformCache[name] = location;
        return location;
    }

    void Shader::setMat4(const char* name, const glm::mat4& value)
    {
        gl()->glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::setMat3(const char* name, const glm::mat3& value)
    {
        gl()->glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    void Shader::setVec2(const char* name, const glm::vec2& value)
    {
        gl()->glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::setVec3(const char* name, const glm::vec3& value)
    {
        gl()->glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::setVec4(const char* name, const glm::vec4& value)
    {
        gl()->glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }

    void Shader::setFloat(const char* name, float value)
    {
        gl()->glUniform1f(getUniformLocation(name), value);
    }

    void Shader::setInt(const char* name, int value)
    {
        gl()->glUniform1i(getUniformLocation(name), value);
    }

    void Shader::setBool(const char* name, bool value)
    {
        gl()->glUniform1i(getUniformLocation(name), value ? 1 : 0);
    }

    void Shader::setMat4Array(const char* name, const glm::mat4* values, int count)
    {
        gl()->glUniformMatrix4fv(getUniformLocation(name), count, GL_FALSE,
                                 glm::value_ptr(values[0]));
    }

} // namespace nwn
