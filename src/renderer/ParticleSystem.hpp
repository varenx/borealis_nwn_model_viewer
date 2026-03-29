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
#include <filesystem>
#include <random>
#include <unordered_map>
#include <vector>
#include "Camera.hpp"
#include "GpuTexture.hpp"
#include "Shader.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    // Individual particle state
    struct Particle
    {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        glm::vec3 initialDirection{0.0f, 0.0f, 1.0f}; // Direction at spawn for billboard_to_local_z
        float age{0.0f};
        float lifetime{1.0f};
        float rotation{0.0f};
        float rotationSpeed{0.0f};
        int frameIndex{0}; // For animated textures
        uint32_t spawnIndex{0}; // Order this particle was spawned (for linked mode)
        bool alive{false};
    };

    // Particle emitter manages a pool of particles
    class ParticleEmitter
    {
    public:
        ParticleEmitter();
        ~ParticleEmitter();

        // Initialize from emitter node data
        void initialize(const mdl::EmitterNode& emitterNode, const glm::mat4& worldTransform);

        // Set animated properties (call before update when animation is playing)
        void setAnimatedProperties(const mdl::AnimatedEmitterProperties* props);

        // Update all particles
        void update(float deltaTime, const glm::mat4& worldTransform);

        // Get particles for rendering
        const std::vector<Particle>& getParticles() const
        {
            return m_Particles;
        }
        const mdl::EmitterNode* getEmitterNode() const
        {
            return m_EmitterNode;
        }

        // Emitter state
        bool isActive() const
        {
            return m_Active;
        }
        void setActive(bool active)
        {
            m_Active = active;
        }

        // For accessing emitter properties during rendering
        auto getTextureName() const -> std::string
        {
            return m_EmitterNode ? m_EmitterNode->texture : std::string{};
        }
        auto getBlendMode() const -> std::string
        {
            return m_EmitterNode ? m_EmitterNode->blend : std::string{};
        }
        auto getRenderMode() const -> std::string
        {
            return m_EmitterNode ? m_EmitterNode->render : std::string{};
        }
        auto getAnimatedProperties() const -> const mdl::AnimatedEmitterProperties*
        {
            return m_AnimatedProps;
        }

        // Get emitter name for animation lookup
        auto getName() const -> std::string
        {
            return m_EmitterNode ? m_EmitterNode->name : std::string{};
        }

    private:
        void spawnParticle(const glm::mat4& worldTransform);
        void updateParticle(Particle& p, float deltaTime);

        // Interpolate value based on particle age
        float interpolateValue(float start, float mid, float end, float t, float percentMid) const;
        glm::vec3 interpolateColor(const glm::vec3& start, const glm::vec3& mid,
                                   const glm::vec3& end, float t, float percentMid) const;

        // Get random value in range
        float randomRange(float min, float max);
        glm::vec3 randomDirection(float spreadDegrees);

        const mdl::EmitterNode* m_EmitterNode{nullptr};
        const mdl::AnimatedEmitterProperties* m_AnimatedProps{nullptr};
        std::vector<Particle> m_Particles;
        bool m_Active{true};

        float m_SpawnAccumulator{0.0f};
        glm::mat4 m_LastWorldTransform{1.0f};
        uint32_t m_NextSpawnIndex{0}; // Counter for linked mode particle ordering

        std::mt19937 m_Rng;
        std::uniform_real_distribution<float> m_Dist{0.0f, 1.0f};
    };

    // GPU resources for particle rendering
    class ParticleRenderer
    {
    public:
        ParticleRenderer();
        ~ParticleRenderer();

        bool initialize();
        void shutdown();

        // Render all active emitters
        void render(const std::vector<ParticleEmitter*>& emitters, const Camera& camera,
                    std::unordered_map<std::string, GpuTexture>& textureCache,
                    const std::filesystem::path& resourcePath);

    private:
        void setupQuadVAO();

        Shader m_ParticleShader;
        uint32_t m_QuadVAO{0};
        uint32_t m_QuadVBO{0};
        bool m_Initialized{false};
    };

} // namespace nwn
