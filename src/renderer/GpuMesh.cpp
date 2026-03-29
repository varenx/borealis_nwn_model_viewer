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
#include "GpuMesh.hpp"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <vector>

namespace nwn
{

    using mdl::MeshData;

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

    GpuMesh::~GpuMesh()
    {
        cleanup();
    }

    GpuMesh::GpuMesh(GpuMesh&& other) noexcept :
        m_Vao(other.m_Vao), m_Vbo(other.m_Vbo), m_Ebo(other.m_Ebo),
        m_IndexCount(other.m_IndexCount), m_Skinned(other.m_Skinned)
    {
        other.m_Vao = 0;
        other.m_Vbo = 0;
        other.m_Ebo = 0;
        other.m_IndexCount = 0;
    }

    GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            m_Vao = other.m_Vao;
            m_Vbo = other.m_Vbo;
            m_Ebo = other.m_Ebo;
            m_IndexCount = other.m_IndexCount;
            m_Skinned = other.m_Skinned;
            other.m_Vao = 0;
            other.m_Vbo = 0;
            other.m_Ebo = 0;
            other.m_IndexCount = 0;
        }
        return *this;
    }

    void GpuMesh::cleanup()
    {
        if (m_Vao != 0)
        {
            gl()->glDeleteVertexArrays(1, &m_Vao);
            m_Vao = 0;
        }
        if (m_Vbo != 0)
        {
            gl()->glDeleteBuffers(1, &m_Vbo);
            m_Vbo = 0;
        }
        if (m_Ebo != 0)
        {
            gl()->glDeleteBuffers(1, &m_Ebo);
            m_Ebo = 0;
        }
    }

    void GpuMesh::upload(const MeshData& mesh)
    {
        cleanup();
        m_Skinned = false;

        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            return;
        }

        // Build interleaved vertex data
        std::vector<Vertex> vertices;
        vertices.reserve(mesh.vertices.size());

        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            Vertex v{};
            v.position = mesh.vertices[i];
            v.normal = i < mesh.normals.size() ? mesh.normals[i] : glm::vec3(0, 0, 1);
            v.texCoord = (!mesh.uvChannels[0].empty() && i < mesh.uvChannels[0].size())
                ? mesh.uvChannels[0][i]
                : glm::vec2(0, 0);

            if (i < mesh.vertexColors.size())
            {
                uint32_t c = mesh.vertexColors[i];
                v.color = glm::vec4((c & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f,
                                    ((c >> 16) & 0xFF) / 255.0f, ((c >> 24) & 0xFF) / 255.0f);
            }
            else
            {
                v.color = glm::vec4(1, 1, 1, 1);
            }

            vertices.push_back(v);
        }

        // Create VAO
        gl()->glGenVertexArrays(1, &m_Vao);
        gl()->glBindVertexArray(m_Vao);

        // Create VBO
        gl()->glGenBuffers(1, &m_Vbo);
        gl()->glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
        gl()->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(),
                           GL_STATIC_DRAW);

        // Create EBO
        gl()->glGenBuffers(1, &m_Ebo);
        gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
        gl()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint16_t),
                           mesh.indices.data(), GL_STATIC_DRAW);

        // Set up vertex attributes
        // Position (location 0)
        gl()->glEnableVertexAttribArray(0);
        gl()->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<void*>(offsetof(Vertex, position)));

        // Normal (location 1)
        gl()->glEnableVertexAttribArray(1);
        gl()->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<void*>(offsetof(Vertex, normal)));

        // TexCoord (location 2)
        gl()->glEnableVertexAttribArray(2);
        gl()->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

        // Color (location 3)
        gl()->glEnableVertexAttribArray(3);
        gl()->glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                    reinterpret_cast<void*>(offsetof(Vertex, color)));

        gl()->glBindVertexArray(0);

        m_IndexCount = static_cast<uint32_t>(mesh.indices.size());
    }

    void GpuMesh::uploadSkinned(const MeshData& mesh, const std::vector<float>& skinWeights,
                                const std::vector<int16_t>& skinBoneRefs)
    {
        cleanup();
        m_Skinned = true;

        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            return;
        }

        // Build interleaved skinned vertex data
        std::vector<SkinnedVertex> vertices;
        vertices.reserve(mesh.vertices.size());

        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            SkinnedVertex v{};
            v.position = mesh.vertices[i];
            v.normal = i < mesh.normals.size() ? mesh.normals[i] : glm::vec3(0, 0, 1);
            v.texCoord = (!mesh.uvChannels[0].empty() && i < mesh.uvChannels[0].size())
                ? mesh.uvChannels[0][i]
                : glm::vec2(0, 0);

            if (i < mesh.vertexColors.size())
            {
                uint32_t c = mesh.vertexColors[i];
                v.color = glm::vec4((c & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f,
                                    ((c >> 16) & 0xFF) / 255.0f, ((c >> 24) & 0xFF) / 255.0f);
            }
            else
            {
                v.color = glm::vec4(1, 1, 1, 1);
            }

            // Bone weights and indices (4 per vertex)
            size_t baseIdx = i * 4;
            if (baseIdx + 4 <= skinWeights.size())
            {
                v.boneWeights = glm::vec4(skinWeights[baseIdx], skinWeights[baseIdx + 1],
                                          skinWeights[baseIdx + 2], skinWeights[baseIdx + 3]);
            }
            else
            {
                v.boneWeights = glm::vec4(1, 0, 0, 0);
            }

            if (baseIdx + 4 <= skinBoneRefs.size())
            {
                v.boneIndices[0] = skinBoneRefs[baseIdx];
                v.boneIndices[1] = skinBoneRefs[baseIdx + 1];
                v.boneIndices[2] = skinBoneRefs[baseIdx + 2];
                v.boneIndices[3] = skinBoneRefs[baseIdx + 3];
            }
            else
            {
                v.boneIndices[0] = v.boneIndices[1] = v.boneIndices[2] = v.boneIndices[3] = 0;
            }

            vertices.push_back(v);
        }

        // Create VAO
        gl()->glGenVertexArrays(1, &m_Vao);
        gl()->glBindVertexArray(m_Vao);

        // Create VBO
        gl()->glGenBuffers(1, &m_Vbo);
        gl()->glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
        gl()->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SkinnedVertex),
                           vertices.data(), GL_STATIC_DRAW);

        // Create EBO
        gl()->glGenBuffers(1, &m_Ebo);
        gl()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
        gl()->glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint16_t),
                           mesh.indices.data(), GL_STATIC_DRAW);

        // Set up vertex attributes
        // Position (location 0)
        gl()->glEnableVertexAttribArray(0);
        gl()->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                                    reinterpret_cast<void*>(offsetof(SkinnedVertex, position)));

        // Normal (location 1)
        gl()->glEnableVertexAttribArray(1);
        gl()->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                                    reinterpret_cast<void*>(offsetof(SkinnedVertex, normal)));

        // TexCoord (location 2)
        gl()->glEnableVertexAttribArray(2);
        gl()->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                                    reinterpret_cast<void*>(offsetof(SkinnedVertex, texCoord)));

        // Color (location 3)
        gl()->glEnableVertexAttribArray(3);
        gl()->glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                                    reinterpret_cast<void*>(offsetof(SkinnedVertex, color)));

        // Bone weights (location 4)
        gl()->glEnableVertexAttribArray(4);
        gl()->glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                                    reinterpret_cast<void*>(offsetof(SkinnedVertex, boneWeights)));

        // Bone indices (location 5)
        gl()->glEnableVertexAttribArray(5);
        gl()->glVertexAttribIPointer(5, 4, GL_INT, sizeof(SkinnedVertex),
                                     reinterpret_cast<void*>(offsetof(SkinnedVertex, boneIndices)));

        gl()->glBindVertexArray(0);

        m_IndexCount = static_cast<uint32_t>(mesh.indices.size());
    }

    void GpuMesh::draw() const
    {
        if (m_Vao == 0)
            return;

        gl()->glBindVertexArray(m_Vao);
        gl()->glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_SHORT, nullptr);
        gl()->glBindVertexArray(0);
    }

    void GpuMesh::drawWireframe() const
    {
        if (m_Vao == 0)
            return;

        gl()->glBindVertexArray(m_Vao);
        gl()->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gl()->glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_SHORT, nullptr);
        gl()->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        gl()->glBindVertexArray(0);
    }

} // namespace nwn
