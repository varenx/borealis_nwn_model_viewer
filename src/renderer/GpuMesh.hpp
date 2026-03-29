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

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    // Interleaved vertex format
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 color; // Vertex color (RGBA normalized)
    };

    // For skinned meshes
    struct SkinnedVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec4 color;
        glm::vec4 boneWeights;
        int boneIndices[4];
    };

    class GpuMesh
    {
    public:
        GpuMesh() = default;
        ~GpuMesh();

        // Non-copyable
        GpuMesh(const GpuMesh&) = delete;
        GpuMesh& operator=(const GpuMesh&) = delete;

        // Movable
        GpuMesh(GpuMesh&& other) noexcept;
        GpuMesh& operator=(GpuMesh&& other) noexcept;

        // Upload mesh data to GPU
        void upload(const mdl::MeshData& mesh);

        // Upload skinned mesh data
        void uploadSkinned(const mdl::MeshData& mesh, const std::vector<float>& skinWeights,
                           const std::vector<int16_t>& skinBoneRefs);

        void draw() const;
        void drawWireframe() const;

        bool isValid() const
        {
            return m_Vao != 0;
        }
        bool isSkinned() const
        {
            return m_Skinned;
        }

    private:
        void cleanup();

        uint32_t m_Vao{0};
        uint32_t m_Vbo{0};
        uint32_t m_Ebo{0};
        uint32_t m_IndexCount{0};
        bool m_Skinned{false};
    };

} // namespace nwn
