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
#pragma once

#include <borealis/nwn/mdl/Mdl.hpp>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nwn
{

    class Shader
    {
    public:
        Shader() = default;
        ~Shader();

        // Non-copyable
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        // Movable
        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        bool compile(std::string_view vertexSource, std::string_view fragmentSource);
        void use() const;
        bool isValid() const
        {
            return m_Program != 0;
        }

        // Uniform setters
        void setMat4(const char* name, const glm::mat4& value);
        void setMat3(const char* name, const glm::mat3& value);
        void setVec2(const char* name, const glm::vec2& value);
        void setVec3(const char* name, const glm::vec3& value);
        void setVec4(const char* name, const glm::vec4& value);
        void setFloat(const char* name, float value);
        void setInt(const char* name, int value);
        void setBool(const char* name, bool value);

        // For skinned meshes - set bone matrices
        void setMat4Array(const char* name, const glm::mat4* values, int count);

    private:
        int32_t getUniformLocation(const char* name) const;
        bool compileShader(uint32_t type, std::string_view source, uint32_t& shader);
        bool linkProgram(uint32_t vertShader, uint32_t fragShader);

        uint32_t m_Program{0};
        mutable std::unordered_map<std::string, int32_t> m_UniformCache;
    };

} // namespace nwn
