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
#include <chrono>
#include <memory>
#include <unordered_map>
#include "Camera.hpp"
#include "GpuMesh.hpp"
#include "GpuTexture.hpp"
#include "ParticleSystem.hpp"
#include "Shader.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    // Renderable node with GPU resources
    struct RenderNode
    {
        mdl::NodeType type{mdl::NodeType::Dummy};
        std::string name;
        glm::mat4 worldTransform{1.0f};
        glm::mat4 bindPoseWorldTransform{1.0f}; // Original bind pose transform (for skinned meshes)
        std::unique_ptr<GpuMesh> gpuMesh;
        GpuTexture* texture{nullptr}; // Borrowed pointer from cache
        mdl::MeshMaterial material;
        bool visible{true};

        // For skinned meshes: bone names indexed by bone slot
        std::vector<std::string> boneNames;
        // Inverse bind pose matrices for each bone (indexed same as boneNames)
        std::vector<glm::mat4> inverseBindPose;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        // Initialize OpenGL resources (call after GL context is created)
        bool initialize();
        void shutdown();

        // Prepare model for rendering (upload to GPU)
        void prepareModel(const mdl::Model& model);
        void clearModel();

        // Render the prepared model
        void render(const Camera& camera);

        // Rendering options
        void setShowMesh(bool show)
        {
            m_ShowMesh = show;
        }
        void setShowWireframe(bool show)
        {
            m_ShowWireframe = show;
        }
        void setBackgroundColor(const glm::vec3& color)
        {
            m_BackgroundColor = color;
        }

        // Node visibility control
        void setNodeVisible(const std::string& nodeName, bool visible);

        // Particle system control
        void setShowParticles(bool show)
        {
            m_ShowParticles = show;
        }
        bool getShowParticles() const
        {
            return m_ShowParticles;
        }

        // Update node transforms from animation player bone matrices
        void updateNodeTransforms(const std::unordered_map<std::string, glm::mat4>& boneMatrices);

        // Set bone matrices for skinned mesh rendering
        void setBoneMatrices(const std::unordered_map<std::string, glm::mat4>* boneMatrices)
        {
            m_CurrentBoneMatrices = boneMatrices;
        }

        // Set animated emitter properties for particle animation
        void setAnimatedEmitterProperties(
            const std::unordered_map<std::string, mdl::AnimatedEmitterProperties>* props)
        {
            m_CurrentEmitterProperties = props;
        }

        // Get bounding box of prepared model
        glm::vec3 getBoundsMin() const
        {
            return m_BoundsMin;
        }
        glm::vec3 getBoundsMax() const
        {
            return m_BoundsMax;
        }

    private:
        // Build render tree from model
        void buildRenderTree(const mdl::Node* node, const glm::mat4& parentTransform);
        void renderNode(const RenderNode& node, const Camera& camera);

        // Compute bind pose transforms for all nodes
        void computeBindPoseTransforms(const mdl::Node* node, const glm::mat4& parentTransform);

        // Build mapping from part number to node name
        void buildPartNumberMap(const mdl::Node* node);

        // Part number to node name mapping (for skinned mesh bone lookup)
        std::unordered_map<int32_t, std::string> m_PartNumberToName;

        // Bind pose world transforms for all nodes (computed during prepareModel)
        std::unordered_map<std::string, glm::mat4> m_BindPoseTransforms;

        // Current bone matrices for skinned mesh rendering (from animation player)
        const std::unordered_map<std::string, glm::mat4>* m_CurrentBoneMatrices{nullptr};

        // Current animated emitter properties (from animation player)
        const std::unordered_map<std::string, mdl::AnimatedEmitterProperties>*
            m_CurrentEmitterProperties{nullptr};

        // Shader sources (embedded)
        static const char* getBasicVertexShader();
        static const char* getBasicFragmentShader();
        static const char* getSkinnedVertexShader();
        static const char* getSkinnedFragmentShader();

        Shader m_BasicShader;
        Shader m_SkinnedShader;
        std::vector<RenderNode> m_RenderNodes;
        std::unordered_map<std::string, GpuTexture> m_TextureCache;
        mdl::TextureLoader m_TextureLoader;

        std::filesystem::path m_ResourcePath;
        glm::vec3 m_BoundsMin{0.0f};
        glm::vec3 m_BoundsMax{0.0f};

        bool m_ShowMesh{true};
        bool m_ShowWireframe{false};
        bool m_ShowParticles{true};
        glm::vec3 m_BackgroundColor{0.2f, 0.2f, 0.2f};

        // Light settings
        glm::vec3 m_LightDir{-0.5f, -1.0f, -0.5f};
        glm::vec3 m_LightColor{1.0f, 1.0f, 1.0f};

        // Particle system
        ParticleRenderer m_ParticleRenderer;
        std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters;
        std::unordered_map<size_t, glm::mat4>
            m_EmitterTransforms; // Emitter index -> world transform
        std::chrono::steady_clock::time_point m_LastFrameTime;
        bool m_FirstFrame{true};
    };

} // namespace nwn
