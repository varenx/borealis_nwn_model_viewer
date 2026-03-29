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
#include "Renderer.hpp"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "animation/AnimationPlayer.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    using mdl::AABBMeshNode;
    using mdl::AnimMeshNode;
    using mdl::DanglyMeshNode;
    using mdl::EmitterNode;
    using mdl::Model;
    using mdl::Node;
    using mdl::NodeType;
    using mdl::SkinMeshNode;
    using mdl::TriMeshNode;

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

    Renderer::Renderer() = default;
    Renderer::~Renderer() = default;

    bool Renderer::initialize()
    {
        // Compile shaders
        if (!m_BasicShader.compile(getBasicVertexShader(), getBasicFragmentShader()))
        {
            spdlog::error("Failed to compile basic shader");
            return false;
        }

        if (!m_SkinnedShader.compile(getSkinnedVertexShader(), getSkinnedFragmentShader()))
        {
            spdlog::error("Failed to compile skinned shader");
            return false;
        }

        // Initialize particle renderer
        if (!m_ParticleRenderer.initialize())
        {
            spdlog::warn("Failed to initialize particle renderer - particles will not be rendered");
        }

        // Enable depth testing
        gl()->glEnable(GL_DEPTH_TEST);
        gl()->glDepthFunc(GL_LESS);

        // Enable back-face culling
        gl()->glEnable(GL_CULL_FACE);
        gl()->glCullFace(GL_BACK);

        // Enable alpha blending
        gl()->glEnable(GL_BLEND);
        gl()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        return true;
    }

    void Renderer::shutdown()
    {
        clearModel();
    }

    void Renderer::prepareModel(const Model& model)
    {
        clearModel();

        m_ResourcePath = model.resourcePath;
        m_BoundsMin = model.boundingBoxMin;
        m_BoundsMax = model.boundingBoxMax;

        spdlog::info("Resource path: {}", m_ResourcePath.string());

        if (model.rootNode)
        {
            // First build part number to name mapping for skinned mesh bone lookup
            m_PartNumberToName.clear();
            buildPartNumberMap(model.rootNode.get());

            // Compute bind pose transforms for all nodes (needed for skinned mesh inverse bind
            // pose)
            m_BindPoseTransforms.clear();
            computeBindPoseTransforms(model.rootNode.get(), glm::mat4(1.0f));

            // Then build the render tree
            buildRenderTree(model.rootNode.get(), glm::mat4(1.0f));
        }

        spdlog::info("Prepared {} render nodes", m_RenderNodes.size());
    }

    void Renderer::computeBindPoseTransforms(const Node* node, const glm::mat4& parentTransform)
    {
        if (!node)
            return;

        glm::mat4 localTransform = node->getLocalTransform();
        glm::mat4 worldTransform = parentTransform * localTransform;

        m_BindPoseTransforms[node->base().name] = worldTransform;

        for (const auto& child : node->base().children)
        {
            computeBindPoseTransforms(child.get(), worldTransform);
        }
    }

    void Renderer::clearModel()
    {
        m_RenderNodes.clear();
        m_TextureCache.clear();
        m_BindPoseTransforms.clear();
        m_Emitters.clear();
        m_EmitterTransforms.clear();
        m_FirstFrame = true;
    }

    void Renderer::buildRenderTree(const Node* node, const glm::mat4& parentTransform)
    {
        glm::mat4 localTransform = node->getLocalTransform();
        glm::mat4 worldTransform = parentTransform * localTransform;

        // Debug: log skinmesh transforms
        if (node->type == NodeType::SkinMesh)
        {
            const auto& b = node->base();
            spdlog::info("SkinMesh '{}' local: pos({:.3f},{:.3f},{:.3f}) "
                         "orient(x={:.3f},y={:.3f},z={:.3f},w={:.3f})",
                         b.name, b.position.x, b.position.y, b.position.z, b.orientation.x,
                         b.orientation.y, b.orientation.z, b.orientation.w);
            spdlog::info("  localTransform rows: [{:.3f},{:.3f},{:.3f}] [{:.3f},{:.3f},{:.3f}] "
                         "[{:.3f},{:.3f},{:.3f}]",
                         localTransform[0][0], localTransform[1][0], localTransform[2][0],
                         localTransform[0][1], localTransform[1][1], localTransform[2][1],
                         localTransform[0][2], localTransform[1][2], localTransform[2][2]);
        }

        // Check if this node has renderable mesh data
        bool hasMesh = std::visit(
            [](const auto& n)
            {
                using T = std::decay_t<decltype(n)>;
                if constexpr (std::is_same_v<T, TriMeshNode> || std::is_same_v<T, SkinMeshNode> ||
                              std::is_same_v<T, AnimMeshNode> ||
                              std::is_same_v<T, DanglyMeshNode> || std::is_same_v<T, AABBMeshNode>)
                {
                    return !n.mesh.vertices.empty();
                }
                return false;
            },
            node->data);

        if (hasMesh)
        {

            RenderNode renderNode;
            renderNode.type = node->type;
            renderNode.name = node->base().name;
            renderNode.worldTransform = worldTransform;
            renderNode.bindPoseWorldTransform = worldTransform; // Store bind pose for skinning

            // Get mesh data and create GPU mesh
            std::visit(
                [&](const auto& n)
                {
                    using T = std::decay_t<decltype(n)>;
                    if constexpr (std::is_same_v<T, TriMeshNode>)
                    {
                        renderNode.material = n.mesh.material;
                        renderNode.gpuMesh = std::make_unique<GpuMesh>();
                        renderNode.gpuMesh->upload(n.mesh);
                    }
                    else if constexpr (std::is_same_v<T, SkinMeshNode>)
                    {
                        renderNode.material = n.mesh.material;
                        renderNode.gpuMesh = std::make_unique<GpuMesh>();

                        // Check if we have ASCII format weights (with bone names) or binary format
                        if (!n.weights.empty() && n.skinWeights.empty())
                        {
                            // ASCII format: convert weights with bone names to GPU format
                            // The mesh may have been expanded, so use originalVertexIndices to map
                            // weights
                            std::unordered_map<std::string, int> boneNameToIndex;

                            // First pass: build the bone name to index mapping
                            for (const auto& w : n.weights)
                            {
                                for (int j = 0; j < 4; j++)
                                {
                                    const auto& boneName = w.boneNames[j];
                                    if (!boneName.empty() && boneName != "NULL" &&
                                        boneName != "null")
                                    {
                                        if (boneNameToIndex.find(boneName) == boneNameToIndex.end())
                                        {
                                            boneNameToIndex[boneName] =
                                                static_cast<int>(boneNameToIndex.size());
                                        }
                                    }
                                }
                            }

                            // Build bone names array from the mapping
                            renderNode.boneNames.resize(boneNameToIndex.size());
                            for (const auto& [name, idx] : boneNameToIndex)
                            {
                                if (idx < static_cast<int>(renderNode.boneNames.size()))
                                {
                                    renderNode.boneNames[idx] = name;
                                }
                            }

                            // Second pass: build weights for each vertex (using original vertex
                            // indices if mesh was expanded)
                            size_t numVertices = n.mesh.vertices.size();
                            std::vector<float> skinWeights;
                            std::vector<int16_t> skinBoneRefs;
                            skinWeights.reserve(numVertices * 4);
                            skinBoneRefs.reserve(numVertices * 4);

                            int zeroWeightVerts = 0;
                            int missingWeightVerts = 0;

                            for (size_t vi = 0; vi < numVertices; vi++)
                            {
                                // Get the original vertex index (before mesh expansion)
                                size_t origVi = vi;
                                if (!n.mesh.originalVertexIndices.empty() &&
                                    vi < n.mesh.originalVertexIndices.size())
                                {
                                    origVi = n.mesh.originalVertexIndices[vi];
                                }

                                // Get weights for this original vertex
                                if (origVi < n.weights.size())
                                {
                                    const auto& w = n.weights[origVi];

                                    // Calculate weight sum for normalization
                                    float weightSum = 0.0f;
                                    for (int j = 0; j < 4; j++)
                                    {
                                        weightSum += w.weights[j];
                                    }

                                    if (weightSum < 0.001f)
                                    {
                                        // Zero weight - use first bone with weight 1
                                        zeroWeightVerts++;
                                        skinWeights.push_back(1.0f);
                                        skinWeights.push_back(0.0f);
                                        skinWeights.push_back(0.0f);
                                        skinWeights.push_back(0.0f);
                                        skinBoneRefs.push_back(0);
                                        skinBoneRefs.push_back(0);
                                        skinBoneRefs.push_back(0);
                                        skinBoneRefs.push_back(0);
                                    }
                                    else
                                    {
                                        // Normalize weights and add them
                                        static int unmappedBoneWarnings = 0;
                                        for (int j = 0; j < 4; j++)
                                        {
                                            const auto& boneName = w.boneNames[j];
                                            int boneIdx = 0;
                                            if (!boneName.empty() && boneName != "NULL" &&
                                                boneName != "null")
                                            {
                                                auto it = boneNameToIndex.find(boneName);
                                                if (it != boneNameToIndex.end())
                                                {
                                                    boneIdx = it->second;
                                                }
                                                else if (w.weights[j] > 0.001f &&
                                                         unmappedBoneWarnings < 10)
                                                {
                                                    // Bone name not found but has non-zero weight!
                                                    spdlog::error(
                                                        "Weight references unmapped bone '{}' with "
                                                        "weight {:.3f} at vertex {}",
                                                        boneName, w.weights[j], vi);
                                                    unmappedBoneWarnings++;
                                                }
                                            }
                                            // Normalize the weight
                                            float normalizedWeight = w.weights[j] / weightSum;
                                            skinWeights.push_back(normalizedWeight);
                                            skinBoneRefs.push_back(static_cast<int16_t>(boneIdx));
                                        }
                                    }
                                }
                                else
                                {
                                    // No weights for this vertex - use first bone with weight 1
                                    missingWeightVerts++;
                                    skinWeights.push_back(1.0f);
                                    skinWeights.push_back(0.0f);
                                    skinWeights.push_back(0.0f);
                                    skinWeights.push_back(0.0f);
                                    skinBoneRefs.push_back(0);
                                    skinBoneRefs.push_back(0);
                                    skinBoneRefs.push_back(0);
                                    skinBoneRefs.push_back(0);
                                }
                            }

                            if (zeroWeightVerts > 0 || missingWeightVerts > 0)
                            {
                                spdlog::warn("SkinMesh '{}': {} vertices with zero weights, {} "
                                             "missing weights",
                                             node->base().name, zeroWeightVerts,
                                             missingWeightVerts);
                            }

                            // Debug info
                            spdlog::info("SkinMesh '{}': {} verts, {} weights, {} bones",
                                         node->base().name, numVertices, n.weights.size(),
                                         boneNameToIndex.size());
                            // Log first few vertex weights and their original indices
                            for (size_t vi = 0; vi < std::min(size_t(3), numVertices); vi++)
                            {
                                size_t base = vi * 4;
                                size_t origVi = vi;
                                if (!n.mesh.originalVertexIndices.empty() &&
                                    vi < n.mesh.originalVertexIndices.size())
                                {
                                    origVi = n.mesh.originalVertexIndices[vi];
                                }
                                spdlog::info("  vert[{}] (orig {}) weights: "
                                             "{:.3f},{:.3f},{:.3f},{:.3f} bones: {},{},{},{}",
                                             vi, origVi, skinWeights[base], skinWeights[base + 1],
                                             skinWeights[base + 2], skinWeights[base + 3],
                                             skinBoneRefs[base], skinBoneRefs[base + 1],
                                             skinBoneRefs[base + 2], skinBoneRefs[base + 3]);
                            }
                            spdlog::info("  originalVertexIndices size: {}",
                                         n.mesh.originalVertexIndices.size());

                            // Compute inverse bind pose matrices for each bone
                            renderNode.inverseBindPose.resize(renderNode.boneNames.size());
                            int missingBones = 0;
                            for (size_t i = 0; i < renderNode.boneNames.size(); i++)
                            {
                                auto it = m_BindPoseTransforms.find(renderNode.boneNames[i]);
                                if (it != m_BindPoseTransforms.end())
                                {
                                    renderNode.inverseBindPose[i] = glm::inverse(it->second);
                                }
                                else
                                {
                                    missingBones++;
                                    spdlog::warn("Bone '{}' not found in model hierarchy",
                                                 renderNode.boneNames[i]);
                                    renderNode.inverseBindPose[i] = glm::mat4(1.0f);
                                }
                            }
                            if (missingBones > 0)
                            {
                                spdlog::warn("SkinMesh '{}': {} bones missing from hierarchy",
                                             node->base().name, missingBones);
                            }

                            renderNode.gpuMesh->uploadSkinned(n.mesh, skinWeights, skinBoneRefs);
                        }
                        else
                        {
                            // Binary format: use existing skinWeights/skinBoneRefs
                            renderNode.gpuMesh->uploadSkinned(n.mesh, n.skinWeights,
                                                              n.skinBoneRefs);

                            // Populate bone names from boneNodeNumbers using part number mapping
                            for (size_t i = 0; i < n.boneNodeNumbers.size(); i++)
                            {
                                int16_t partNum = n.boneNodeNumbers[i];
                                if (partNum >= 0)
                                {
                                    auto it = m_PartNumberToName.find(partNum);
                                    if (it != m_PartNumberToName.end())
                                    {
                                        if (renderNode.boneNames.size() <= i)
                                        {
                                            renderNode.boneNames.resize(i + 1);
                                        }
                                        renderNode.boneNames[i] = it->second;
                                    }
                                }
                            }

                            // Compute inverse bind pose matrices for each bone
                            renderNode.inverseBindPose.resize(renderNode.boneNames.size());
                            for (size_t i = 0; i < renderNode.boneNames.size(); i++)
                            {
                                auto it = m_BindPoseTransforms.find(renderNode.boneNames[i]);
                                if (it != m_BindPoseTransforms.end())
                                {
                                    renderNode.inverseBindPose[i] = glm::inverse(it->second);
                                }
                                else
                                {
                                    renderNode.inverseBindPose[i] = glm::mat4(1.0f);
                                }
                            }
                        }
                    }
                    else if constexpr (std::is_same_v<T, AnimMeshNode> ||
                                       std::is_same_v<T, DanglyMeshNode> ||
                                       std::is_same_v<T, AABBMeshNode>)
                    {
                        renderNode.material = n.mesh.material;
                        renderNode.gpuMesh = std::make_unique<GpuMesh>();
                        renderNode.gpuMesh->upload(n.mesh);
                    }
                },
                node->data);

            // Load texture if specified (skip "null" textures)
            const std::string& texName = renderNode.material.textures[0];
            // Normalize texture name to lowercase for case-insensitive lookup
            std::string texKey = texName;
            std::transform(texKey.begin(), texKey.end(), texKey.begin(), ::tolower);
            if (!texKey.empty() && texKey != "null")
            {
                auto it = m_TextureCache.find(texKey);
                if (it == m_TextureCache.end())
                {
                    auto result = m_TextureLoader.findAndLoad(texName, m_ResourcePath);
                    if (result)
                    {
                        GpuTexture gpuTex;
                        gpuTex.upload(*result);
                        m_TextureCache[texKey] = std::move(gpuTex);
                        renderNode.texture = &m_TextureCache[texKey];
                    }
                    else
                    {
                        spdlog::warn("Failed to load texture: {}", texName);
                    }
                }
                else
                {
                    renderNode.texture = &it->second;
                }
            }

            m_RenderNodes.push_back(std::move(renderNode));
        }

        // Check if this is an emitter node and create particle emitter
        if (node->type == NodeType::Emitter)
        {
            const auto* emitterData = std::get_if<EmitterNode>(&node->data);
            if (emitterData)
            {
                auto emitter = std::make_unique<ParticleEmitter>();
                emitter->initialize(*emitterData, worldTransform);
                // Store the world transform for updates
                m_EmitterTransforms[m_Emitters.size()] = worldTransform;
                m_Emitters.push_back(std::move(emitter));
                spdlog::info("Created particle emitter: texture='{}' birthRate={} lifeExp={} "
                             "pos=({:.2f},{:.2f},{:.2f})",
                             emitterData->texture, emitterData->birthRate, emitterData->lifeExp,
                             worldTransform[3][0], worldTransform[3][1], worldTransform[3][2]);
            }
        }

        // Process children
        for (const auto& child : node->base().children)
        {
            buildRenderTree(child.get(), worldTransform);
        }
    }

    void Renderer::render(const Camera& camera)
    {
        gl()->glClearColor(m_BackgroundColor.x, m_BackgroundColor.y, m_BackgroundColor.z, 1.0f);
        gl()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate delta time
        auto now = std::chrono::steady_clock::now();
        float deltaTime = 0.0f;
        if (!m_FirstFrame)
        {
            deltaTime = std::chrono::duration<float>(now - m_LastFrameTime).count();
            // Clamp to avoid huge jumps
            deltaTime = std::min(deltaTime, 0.1f);
        }
        m_LastFrameTime = now;
        m_FirstFrame = false;

        if (m_RenderNodes.empty() && m_Emitters.empty())
        {
            return;
        }

        for (const auto& node : m_RenderNodes)
        {
            if (node.visible && m_ShowMesh)
            {
                renderNode(node, camera);
            }
        }

        if (m_ShowWireframe)
        {
            // Render wireframe overlay
            for (const auto& node : m_RenderNodes)
            {
                if (node.visible && node.gpuMesh)
                {
                    m_BasicShader.use();
                    m_BasicShader.setMat4("uModel", node.worldTransform);
                    m_BasicShader.setMat4("uView", camera.getViewMatrix());
                    m_BasicShader.setMat4("uProjection", camera.getProjectionMatrix());
                    m_BasicShader.setMat3("uNormalMatrix", glm::mat3(1.0f));
                    m_BasicShader.setVec3("uDiffuse", glm::vec3(0.0f, 0.0f, 0.0f));
                    m_BasicShader.setVec3("uAmbient", glm::vec3(1.0f, 1.0f, 1.0f));
                    m_BasicShader.setVec3("uSpecular", glm::vec3(0.0f));
                    m_BasicShader.setFloat("uShininess", 1.0f);
                    m_BasicShader.setFloat("uAlpha", 1.0f);
                    m_BasicShader.setBool("uHasTexture", false);
                    m_BasicShader.setVec3("uLightDir", m_LightDir);
                    m_BasicShader.setVec3("uLightColor", m_LightColor);
                    m_BasicShader.setVec3("uViewPos", camera.getPosition());

                    node.gpuMesh->drawWireframe();
                }
            }
        }

        // Update and render particles
        if (m_ShowParticles && !m_Emitters.empty())
        {
            // Update particle emitters with their world transforms and animated properties
            for (size_t i = 0; i < m_Emitters.size(); ++i)
            {
                auto it = m_EmitterTransforms.find(i);
                glm::mat4 transform =
                    (it != m_EmitterTransforms.end()) ? it->second : glm::mat4(1.0f);

                // Look up animated properties by emitter name
                const mdl::AnimatedEmitterProperties* animProps = nullptr;
                if (m_CurrentEmitterProperties && !m_CurrentEmitterProperties->empty())
                {
                    const std::string& emitterName = m_Emitters[i]->getName();
                    auto propIt = m_CurrentEmitterProperties->find(emitterName);
                    if (propIt != m_CurrentEmitterProperties->end())
                    {
                        animProps = &propIt->second;
                    }
                }
                m_Emitters[i]->setAnimatedProperties(animProps);
                m_Emitters[i]->update(deltaTime, transform);
            }

            // Collect emitter pointers for rendering
            std::vector<ParticleEmitter*> emitterPtrs;
            emitterPtrs.reserve(m_Emitters.size());
            for (const auto& e : m_Emitters)
            {
                emitterPtrs.push_back(e.get());
            }

            // Render particles
            m_ParticleRenderer.render(emitterPtrs, camera, m_TextureCache, m_ResourcePath);
        }
    }

    void Renderer::setNodeVisible(const std::string& nodeName, bool visible)
    {
        for (auto& node : m_RenderNodes)
        {
            if (node.name == nodeName)
            {
                node.visible = visible;
                return;
            }
        }
    }

    void
    Renderer::updateNodeTransforms(const std::unordered_map<std::string, glm::mat4>& boneMatrices)
    {
        for (auto& node : m_RenderNodes)
        {
            auto it = boneMatrices.find(node.name);
            if (it != boneMatrices.end())
            {
                node.worldTransform = it->second;
            }
        }
    }

    void Renderer::buildPartNumberMap(const Node* node)
    {
        if (!node)
            return;

        const auto& base = node->base();
        if (base.partNumber >= 0)
        {
            m_PartNumberToName[base.partNumber] = base.name;
        }

        for (const auto& child : base.children)
        {
            buildPartNumberMap(child.get());
        }
    }

    void Renderer::renderNode(const RenderNode& node, const Camera& camera)
    {
        if (!node.gpuMesh || !node.gpuMesh->isValid())
        {
            return;
        }

        Shader* shader = node.gpuMesh->isSkinned() ? &m_SkinnedShader : &m_BasicShader;
        shader->use();

        // Set matrices - for skinned meshes, use identity and handle transform in bone matrices
        glm::mat4 modelMatrix = node.gpuMesh->isSkinned() ? glm::mat4(1.0f) : node.worldTransform;
        shader->setMat4("uModel", modelMatrix);
        shader->setMat4("uView", camera.getViewMatrix());
        shader->setMat4("uProjection", camera.getProjectionMatrix());

        glm::mat3 normalMatrix = node.gpuMesh->isSkinned()
            ? glm::mat3(1.0f)
            : glm::transpose(glm::inverse(glm::mat3(node.worldTransform)));
        shader->setMat3("uNormalMatrix", normalMatrix);

        // Set material properties
        shader->setVec3("uDiffuse", node.material.diffuse);
        shader->setVec3("uAmbient", node.material.ambient);
        shader->setVec3("uSpecular", node.material.specular);
        shader->setFloat("uShininess", node.material.shininess);
        shader->setFloat("uAlpha", 1.0f);

        // Set texture
        if (node.texture && node.texture->isValid())
        {
            node.texture->bind(0);
            shader->setInt("uTexture", 0);
            shader->setBool("uHasTexture", true);
        }
        else
        {
            shader->setBool("uHasTexture", false);
        }

        // Set lighting
        shader->setVec3("uLightDir", glm::normalize(m_LightDir));
        shader->setVec3("uLightColor", m_LightColor);
        shader->setVec3("uViewPos", camera.getPosition());

        // For skinned meshes, set bone matrices from animation
        if (node.gpuMesh->isSkinned())
        {
            constexpr int MAX_BONES = 64;
            glm::mat4 boneMatrices[MAX_BONES];

            // Initialize to identity
            for (int i = 0; i < MAX_BONES; i++)
            {
                boneMatrices[i] = glm::mat4(1.0f);
            }

            // Compute bone matrices that transform from skinmesh local space to animated world
            // space Formula: boneAnimated * inverse(boneBind) * skinmeshBindPose At bind pose:
            // boneBind * inverse(boneBind) * skinmeshBindPose = skinmeshBindPose (correct!)
            int foundBones = 0;
            int missingBones = 0;
            static int logCounter = 0;
            bool shouldLog = (logCounter++ % 300 == 0); // Log every 300 frames
            if (m_CurrentBoneMatrices)
            {
                for (size_t i = 0; i < node.boneNames.size() && i < MAX_BONES; i++)
                {
                    const auto& boneName = node.boneNames[i];
                    if (!boneName.empty())
                    {
                        auto it = m_CurrentBoneMatrices->find(boneName);
                        if (it != m_CurrentBoneMatrices->end())
                        {
                            glm::mat4 invBindPose = (i < node.inverseBindPose.size())
                                ? node.inverseBindPose[i]
                                : glm::mat4(1.0f);
                            boneMatrices[i] =
                                it->second * invBindPose * node.bindPoseWorldTransform;
                            foundBones++;
                        }
                        else
                        {
                            missingBones++;
                            if (shouldLog)
                            {
                                spdlog::warn("Bone '{}' not found in m_CurrentBoneMatrices",
                                             boneName);
                            }
                        }
                    }
                }
            }
            if (missingBones > 0)
            {
                static int missLogCount = 0;
                if (missLogCount++ < 5)
                {
                    spdlog::warn("SkinMesh '{}': {} bones found, {} missing", node.name, foundBones,
                                 missingBones);
                    // Log all missing bones
                    for (size_t i = 0; i < node.boneNames.size() && i < MAX_BONES; i++)
                    {
                        const auto& boneName = node.boneNames[i];
                        if (!boneName.empty())
                        {
                            auto it = m_CurrentBoneMatrices->find(boneName);
                            if (it == m_CurrentBoneMatrices->end())
                            {
                                spdlog::warn("  Missing bone[{}]: '{}'", i, boneName);
                            }
                        }
                    }
                }
            }

            shader->setMat4Array("uBoneMatrices", boneMatrices, MAX_BONES);
        }

        node.gpuMesh->draw();

        if (node.texture)
        {
            node.texture->unbind();
        }
    }

    // Embedded shader sources
    const char* Renderer::getBasicVertexShader()
    {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;
out vec3 vFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uProjection * uView * worldPos;
    vFragPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";
    }

    const char* Renderer::getBasicFragmentShader()
    {
        return R"(
#version 330 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;
in vec3 vFragPos;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec3 uDiffuse;
uniform vec3 uAmbient;
uniform vec3 uSpecular;
uniform float uShininess;
uniform float uAlpha;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

void main() {
    vec3 normal = normalize(vNormal);
    if (!gl_FrontFacing) normal = -normal;

    vec3 ambient = uAmbient * uLightColor * 0.3;
    float diff = max(dot(normal, -uLightDir), 0.0);
    vec3 diffuse = diff * uDiffuse * uLightColor;

    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfDir = normalize(-uLightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), max(uShininess, 1.0));
    vec3 specular = spec * uSpecular * uLightColor;

    vec3 result = ambient + diffuse + specular;
    vec4 baseColor = vColor;
    if (uHasTexture) baseColor *= texture(uTexture, vTexCoord);
    fragColor = vec4(result * baseColor.rgb, baseColor.a * uAlpha);
}
)";
    }

    const char* Renderer::getSkinnedVertexShader()
    {
        return R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;
layout(location = 4) in vec4 aBoneWeights;
layout(location = 5) in ivec4 aBoneIndices;

out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;
out vec3 vFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uBoneMatrices[64];

void main() {
    mat4 skinMatrix =
        aBoneWeights.x * uBoneMatrices[aBoneIndices.x] +
        aBoneWeights.y * uBoneMatrices[aBoneIndices.y] +
        aBoneWeights.z * uBoneMatrices[aBoneIndices.z] +
        aBoneWeights.w * uBoneMatrices[aBoneIndices.w];

    vec4 skinnedPos = skinMatrix * vec4(aPosition, 1.0);
    vec4 worldPos = uModel * skinnedPos;
    gl_Position = uProjection * uView * worldPos;
    vFragPos = worldPos.xyz;
    vNormal = mat3(skinMatrix) * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
}
)";
    }

    const char* Renderer::getSkinnedFragmentShader()
    {
        return getBasicFragmentShader(); // Same as basic
    }

} // namespace nwn
