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
#include "ParticleSystem.hpp"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include <unordered_set>
#include "animation/AnimationPlayer.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    using mdl::AnimatedEmitterProperties;
    using mdl::EmitterNode;
    using mdl::TextureLoader;

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

    // ParticleEmitter implementation

    ParticleEmitter::ParticleEmitter()
    {
        std::random_device rd;
        m_Rng.seed(rd());
    }

    ParticleEmitter::~ParticleEmitter() = default;

    void ParticleEmitter::initialize(const EmitterNode& emitterNode,
                                     const glm::mat4& worldTransform)
    {
        m_EmitterNode = &emitterNode;
        m_LastWorldTransform = worldTransform;
        m_NextSpawnIndex = 0;

        float effectiveLifeExp = emitterNode.lifeExp;
        if (effectiveLifeExp <= 0.0f)
        {
            if (emitterNode.lifeExpController && !emitterNode.lifeExpController->keyframes.empty())
            {
                for (const auto& kf : emitterNode.lifeExpController->keyframes)
                {
                    effectiveLifeExp = std::max(effectiveLifeExp, kf.value);
                }
            }
            if (effectiveLifeExp <= 0.0f)
            {
                spdlog::warn("Emitter '{}' has lifeExp=0, particles won't be visible",
                             emitterNode.texture);
                return;
            }
        }

        float maxBirthRate = emitterNode.birthRate;
        if (emitterNode.birthRateController && !emitterNode.birthRateController->keyframes.empty())
        {
            for (const auto& kf : emitterNode.birthRateController->keyframes)
            {
                maxBirthRate = std::max(maxBirthRate, kf.value);
            }
        }

        if (maxBirthRate <= 0.0f)
        {
            maxBirthRate = 20.0f;
        }

        size_t maxParticles =
            static_cast<size_t>(std::max(1.0f, maxBirthRate * effectiveLifeExp * 1.5f));
        maxParticles = std::min(maxParticles, size_t(10000));
        m_Particles.resize(maxParticles);

        spdlog::info("ParticleEmitter initialized: texture='{}' birthRate={} (max={}) lifeExp={} "
                     "size={} maxParticles={}",
                     emitterNode.texture, emitterNode.birthRate, maxBirthRate, effectiveLifeExp,
                     emitterNode.sizeStart, maxParticles);
    }

    void ParticleEmitter::setAnimatedProperties(const AnimatedEmitterProperties* props)
    {
        m_AnimatedProps = props;
    }

    void ParticleEmitter::update(float deltaTime, const glm::mat4& worldTransform)
    {
        if (!m_EmitterNode || !m_Active || m_Particles.empty())
            return;

        m_LastWorldTransform = worldTransform;

        for (auto& p : m_Particles)
        {
            if (p.alive)
            {
                updateParticle(p, deltaTime);
            }
        }

        float birthRate = m_AnimatedProps ? m_AnimatedProps->birthRate : m_EmitterNode->birthRate;

        if (birthRate > 0)
        {
            m_SpawnAccumulator += deltaTime * birthRate;

            while (m_SpawnAccumulator >= 1.0f)
            {
                spawnParticle(worldTransform);
                m_SpawnAccumulator -= 1.0f;
            }
        }
    }

    void ParticleEmitter::spawnParticle(const glm::mat4& worldTransform)
    {
        Particle* p = nullptr;
        for (auto& particle : m_Particles)
        {
            if (!particle.alive)
            {
                p = &particle;
                break;
            }
        }

        if (!p)
            return;

        float lifeExp = m_AnimatedProps ? m_AnimatedProps->lifeExp : m_EmitterNode->lifeExp;
        float xSize = m_AnimatedProps ? m_AnimatedProps->xSize : m_EmitterNode->xSize;
        float ySize = m_AnimatedProps ? m_AnimatedProps->ySize : m_EmitterNode->ySize;
        float spread = m_AnimatedProps ? m_AnimatedProps->spread : m_EmitterNode->spread;
        float velocity = m_AnimatedProps ? m_AnimatedProps->velocity : m_EmitterNode->velocity;
        float randVel = m_AnimatedProps ? m_AnimatedProps->randVel : m_EmitterNode->randVel;
        float particleRot =
            m_AnimatedProps ? m_AnimatedProps->particleRot : m_EmitterNode->particleRot;

        p->alive = true;
        p->age = 0.0f;
        p->lifetime = lifeExp;
        p->spawnIndex = m_NextSpawnIndex++;

        float halfX = xSize * 0.5f * 0.01f;
        float halfY = ySize * 0.5f * 0.01f;
        glm::vec3 localPos(randomRange(-halfX, halfX), randomRange(-halfY, halfY), 0.0f);

        glm::vec4 worldPos = worldTransform * glm::vec4(localPos, 1.0f);
        p->position = glm::vec3(worldPos);

        glm::vec3 dir = randomDirection(spread);

        glm::mat3 rotMat = glm::mat3(worldTransform);
        dir = glm::normalize(rotMat * dir);

        p->initialDirection = dir;

        float speed = velocity + randomRange(0.0f, randVel);
        p->velocity = dir * speed;

        p->rotation = 0.0f;
        p->rotationSpeed = particleRot * 2.0f * glm::pi<float>();

        p->frameIndex = static_cast<int>(m_EmitterNode->frameStart);
    }

    void ParticleEmitter::updateParticle(Particle& p, float deltaTime)
    {
        p.age += deltaTime;

        if (p.age >= p.lifetime)
        {
            p.alive = false;
            return;
        }

        float mass = m_AnimatedProps ? m_AnimatedProps->mass : m_EmitterNode->mass;
        float grav = m_AnimatedProps ? m_AnimatedProps->grav : m_EmitterNode->grav;
        float fps = m_AnimatedProps ? m_AnimatedProps->fps : m_EmitterNode->fps;

        if (mass != 0.0f)
        {
            p.velocity.z -= mass * 9.81f * deltaTime;
        }

        if (grav != 0.0f)
        {
            glm::vec3 toOrigin = glm::vec3(m_LastWorldTransform[3]) - p.position;
            float dist = glm::length(toOrigin);
            if (dist > 0.01f)
            {
                p.velocity += glm::normalize(toOrigin) * grav * deltaTime;
            }
        }

        p.position += p.velocity * deltaTime;

        p.rotation += p.rotationSpeed * deltaTime;

        if (fps > 0 && m_EmitterNode->frameEnd > m_EmitterNode->frameStart)
        {
            float frameTime = 1.0f / fps;
            int totalFrames =
                static_cast<int>(m_EmitterNode->frameEnd - m_EmitterNode->frameStart + 1);
            int frame = static_cast<int>(p.age / frameTime) % totalFrames;
            p.frameIndex = static_cast<int>(m_EmitterNode->frameStart) + frame;
        }
    }

    float ParticleEmitter::interpolateValue(float start, float mid, float end, float t,
                                            float percentMid) const
    {
        if (t <= percentMid)
        {
            float localT = (percentMid > 0.0f) ? t / percentMid : 0.0f;
            return glm::mix(start, mid, localT);
        }
        else
        {
            float localT =
                (1.0f - percentMid > 0.0f) ? (t - percentMid) / (1.0f - percentMid) : 1.0f;
            return glm::mix(mid, end, localT);
        }
    }

    glm::vec3 ParticleEmitter::interpolateColor(const glm::vec3& start, const glm::vec3& mid,
                                                const glm::vec3& end, float t,
                                                float percentMid) const
    {
        if (t <= percentMid)
        {
            float localT = (percentMid > 0.0f) ? t / percentMid : 0.0f;
            return glm::mix(start, mid, localT);
        }
        else
        {
            float localT =
                (1.0f - percentMid > 0.0f) ? (t - percentMid) / (1.0f - percentMid) : 1.0f;
            return glm::mix(mid, end, localT);
        }
    }

    float ParticleEmitter::randomRange(float min, float max)
    {
        return min + m_Dist(m_Rng) * (max - min);
    }

    glm::vec3 ParticleEmitter::randomDirection(float spreadDegrees)
    {
        if (spreadDegrees <= 0.0f)
        {
            return glm::vec3(0.0f, 0.0f, 1.0f);
        }

        float halfAngle = glm::radians(spreadDegrees * 0.5f);

        float phi = m_Dist(m_Rng) * 2.0f * glm::pi<float>();

        float cosTheta = 1.0f - m_Dist(m_Rng) * (1.0f - std::cos(halfAngle));
        float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);

        return glm::vec3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
    }

    // ParticleRenderer implementation

    static int getRenderModeIndex(const std::string& mode)
    {
        std::string lowerMode = mode;
        std::transform(lowerMode.begin(), lowerMode.end(), lowerMode.begin(), ::tolower);

        if (lowerMode == "normal")
            return 0;
        if (lowerMode == "linked")
            return 1;
        if (lowerMode == "billboard_to_local_z")
            return 2;
        if (lowerMode == "billboard_to_world_z")
            return 3;
        if (lowerMode == "aligned_to_world_z")
            return 4;
        if (lowerMode == "aligned_to_particle_dir")
            return 5;
        if (lowerMode == "motion_blur")
            return 6;

        return 0;
    }

    ParticleRenderer::ParticleRenderer() = default;

    ParticleRenderer::~ParticleRenderer()
    {
        shutdown();
    }

    bool ParticleRenderer::initialize()
    {
        const char* vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec4 vColor;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uParticlePos;
uniform float uParticleSize;
uniform float uParticleRotation;
uniform vec4 uParticleColor;
uniform vec2 uTexOffset;
uniform vec2 uTexScale;
uniform int uRenderMode;
uniform vec3 uParticleVelocity;
uniform vec3 uParticleDirection;
uniform float uMotionBlurStretch;
uniform vec3 uPrevParticlePos;
uniform bool uHasChainTarget;

void main() {
    vec3 right;
    vec3 up;
    float sizeX = uParticleSize;
    float sizeY = uParticleSize;

    vec3 cameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 cameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);
    vec3 cameraForward = vec3(uView[0][2], uView[1][2], uView[2][2]);

    if (uRenderMode == 0) {
        right = cameraRight;
        up = cameraUp;
    }
    else if (uRenderMode == 1) {
        if (uHasChainTarget) {
            vec3 chainDir = uParticlePos - uPrevParticlePos;
            float chainLength = length(chainDir);
            if (chainLength > 0.001) {
                vec3 forward = chainDir / chainLength;
                right = normalize(cross(forward, cameraForward));
                if (length(right) < 0.001) {
                    right = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
                }
                vec3 midpoint = (uPrevParticlePos + uParticlePos) * 0.5;
                float c = cos(uParticleRotation);
                float s = sin(uParticleRotation);
                vec2 rotatedPos = vec2(
                    aPosition.x * c - aPosition.y * s,
                    aPosition.x * s + aPosition.y * c
                );
                vec3 worldPos = midpoint
                    + right * rotatedPos.x * sizeX
                    + forward * rotatedPos.y * chainLength * 0.5;
                gl_Position = uProjection * uView * vec4(worldPos, 1.0);
                vTexCoord = aTexCoord * uTexScale + uTexOffset;
                vColor = uParticleColor;
                return;
            }
        }
        right = cameraRight;
        up = cameraUp;
    }
    else if (uRenderMode == 2) {
        vec3 dir = normalize(uParticleDirection);
        right = normalize(cross(vec3(0.0, 0.0, 1.0), dir));
        if (length(right) < 0.001) {
            right = normalize(cross(vec3(0.0, 1.0, 0.0), dir));
        }
        up = normalize(cross(dir, right));
    }
    else if (uRenderMode == 3) {
        vec3 toCamera = -cameraForward;
        toCamera.z = 0.0;
        if (length(toCamera) < 0.001) {
            toCamera = vec3(1.0, 0.0, 0.0);
        }
        toCamera = normalize(toCamera);
        right = normalize(cross(vec3(0.0, 0.0, 1.0), toCamera));
        up = toCamera;
    }
    else if (uRenderMode == 4) {
        up = vec3(0.0, 0.0, 1.0);
        vec3 toCamera = -cameraForward;
        toCamera.z = 0.0;
        if (length(toCamera) < 0.001) {
            toCamera = vec3(1.0, 0.0, 0.0);
        }
        right = normalize(toCamera);
    }
    else if (uRenderMode == 5) {
        vec3 vel = uParticleVelocity;
        float speed = length(vel);
        if (speed > 0.001) {
            vec3 dir = vel / speed;
            up = dir;
            right = normalize(cross(up, cameraForward));
            if (length(right) < 0.001) {
                right = normalize(cross(up, vec3(0.0, 1.0, 0.0)));
            }
        } else {
            right = cameraRight;
            up = cameraUp;
        }
    }
    else if (uRenderMode == 6) {
        vec3 vel = uParticleVelocity;
        float speed = length(vel);
        if (speed > 0.001) {
            vec3 dir = vel / speed;
            up = dir;
            right = normalize(cross(up, cameraForward));
            if (length(right) < 0.001) {
                right = normalize(cross(up, vec3(0.0, 1.0, 0.0)));
            }
            sizeY = uParticleSize * (1.0 + speed * uMotionBlurStretch);
        } else {
            right = cameraRight;
            up = cameraUp;
        }
    }
    else {
        right = cameraRight;
        up = cameraUp;
    }

    float c = cos(uParticleRotation);
    float s = sin(uParticleRotation);
    vec2 rotatedPos = vec2(
        aPosition.x * c - aPosition.y * s,
        aPosition.x * s + aPosition.y * c
    );

    vec3 worldPos = uParticlePos
        + right * rotatedPos.x * sizeX
        + up * rotatedPos.y * sizeY;

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);

    vTexCoord = aTexCoord * uTexScale + uTexOffset;
    vColor = uParticleColor;
}
)";

        const char* fragmentShader = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform bool uPunchThrough;

void main() {
    vec4 texColor = uHasTexture ? texture(uTexture, vTexCoord) : vec4(1.0);
    fragColor = texColor * vColor;

    if (uPunchThrough) {
        if (fragColor.a < 0.5) discard;
        fragColor.a = 1.0;
    } else {
        if (fragColor.a < 0.01) discard;
    }
}
)";

        if (!m_ParticleShader.compile(vertexShader, fragmentShader))
        {
            spdlog::error("Failed to compile particle shader");
            return false;
        }

        setupQuadVAO();
        m_Initialized = true;
        return true;
    }

    void ParticleRenderer::shutdown()
    {
        if (m_QuadVAO)
        {
            gl()->glDeleteVertexArrays(1, &m_QuadVAO);
            m_QuadVAO = 0;
        }
        if (m_QuadVBO)
        {
            gl()->glDeleteBuffers(1, &m_QuadVBO);
            m_QuadVBO = 0;
        }
        m_Initialized = false;
    }

    void ParticleRenderer::setupQuadVAO()
    {
        float quadVertices[] = {
            -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f,  0.5f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.5f,  1.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f,
        };

        gl()->glGenVertexArrays(1, &m_QuadVAO);
        gl()->glGenBuffers(1, &m_QuadVBO);

        gl()->glBindVertexArray(m_QuadVAO);
        gl()->glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        gl()->glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        gl()->glEnableVertexAttribArray(0);
        gl()->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        gl()->glEnableVertexAttribArray(1);
        gl()->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                    (void*)(2 * sizeof(float)));

        gl()->glBindVertexArray(0);
    }

    void ParticleRenderer::render(const std::vector<ParticleEmitter*>& emitters,
                                  const Camera& camera,
                                  std::unordered_map<std::string, GpuTexture>& textureCache,
                                  const std::filesystem::path& resourcePath)
    {
        if (!m_Initialized || emitters.empty())
            return;

        GLboolean depthMask;
        gl()->glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

        gl()->glDepthMask(GL_FALSE);

        m_ParticleShader.use();
        m_ParticleShader.setMat4("uView", camera.getViewMatrix());
        m_ParticleShader.setMat4("uProjection", camera.getProjectionMatrix());

        gl()->glBindVertexArray(m_QuadVAO);

        TextureLoader texLoader;

        for (const auto* emitter : emitters)
        {
            if (!emitter || !emitter->isActive())
                continue;

            const EmitterNode* node = emitter->getEmitterNode();
            if (!node)
                continue;

            std::string blendMode = node->blend;
            std::transform(blendMode.begin(), blendMode.end(), blendMode.begin(), ::tolower);

            bool isPunchThrough = false;
            if (blendMode == "lighten")
            {
                gl()->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
            else if (blendMode == "punch-through")
            {
                gl()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                isPunchThrough = true;
            }
            else
            {
                gl()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            m_ParticleShader.setBool("uPunchThrough", isPunchThrough);

            GpuTexture* texture = nullptr;
            if (!node->texture.empty() && node->texture != "null" && node->texture != "NULL")
            {
                std::string texKey = node->texture;
                std::transform(texKey.begin(), texKey.end(), texKey.begin(), ::tolower);

                auto it = textureCache.find(texKey);
                if (it == textureCache.end())
                {
                    auto result = texLoader.findAndLoad(node->texture, resourcePath);
                    if (result)
                    {
                        GpuTexture gpuTex;
                        gpuTex.upload(*result);
                        textureCache[texKey] = std::move(gpuTex);
                        texture = &textureCache[texKey];
                    }
                    else
                    {
                        static std::unordered_set<std::string> warnedTextures;
                        if (warnedTextures.find(texKey) == warnedTextures.end())
                        {
                            spdlog::warn("Failed to load particle texture: {}", node->texture);
                            warnedTextures.insert(texKey);
                        }
                    }
                }
                else
                {
                    texture = &it->second;
                }
            }

            if (texture && texture->isValid())
            {
                texture->bind(0);
                m_ParticleShader.setInt("uTexture", 0);
                m_ParticleShader.setBool("uHasTexture", true);
            }
            else
            {
                m_ParticleShader.setBool("uHasTexture", false);
            }

            float texScaleX = 1.0f / static_cast<float>(std::max(1u, node->xGrid));
            float texScaleY = 1.0f / static_cast<float>(std::max(1u, node->yGrid));
            m_ParticleShader.setVec2("uTexScale", glm::vec2(texScaleX, texScaleY));

            int renderMode = getRenderModeIndex(node->render);
            m_ParticleShader.setInt("uRenderMode", renderMode);

            m_ParticleShader.setFloat("uMotionBlurStretch", 0.5f);

            const AnimatedEmitterProperties* animProps = emitter->getAnimatedProperties();

            glm::vec3 colorStart = animProps ? animProps->colorStart : node->colorStart;
            glm::vec3 colorMid = animProps ? animProps->colorMid : node->colorMid;
            glm::vec3 colorEnd = animProps ? animProps->colorEnd : node->colorEnd;
            float alphaStart = animProps ? animProps->alphaStart : node->alphaStart;
            float alphaMid = animProps ? animProps->alphaMid : node->alphaMid;
            float alphaEnd = animProps ? animProps->alphaEnd : node->alphaEnd;
            float sizeStart = animProps ? animProps->sizeStart : node->sizeStart;
            float sizeMid = animProps ? animProps->sizeMid : node->sizeMid;
            float sizeEnd = animProps ? animProps->sizeEnd : node->sizeEnd;

            auto renderParticle =
                [&](const Particle& p, bool hasChainTarget, const glm::vec3& prevPos)
            {
                float t = p.age / p.lifetime;

                float size = sizeStart;
                if (t <= node->percentMid)
                {
                    float localT = (node->percentMid > 0.0f) ? t / node->percentMid : 0.0f;
                    size = glm::mix(sizeStart, sizeMid, localT);
                }
                else
                {
                    float localT = (1.0f - node->percentMid > 0.0f)
                        ? (t - node->percentMid) / (1.0f - node->percentMid)
                        : 1.0f;
                    size = glm::mix(sizeMid, sizeEnd, localT);
                }

                glm::vec3 color;
                if (t <= node->percentMid)
                {
                    float localT = (node->percentMid > 0.0f) ? t / node->percentMid : 0.0f;
                    color = glm::mix(colorStart, colorMid, localT);
                }
                else
                {
                    float localT = (1.0f - node->percentMid > 0.0f)
                        ? (t - node->percentMid) / (1.0f - node->percentMid)
                        : 1.0f;
                    color = glm::mix(colorMid, colorEnd, localT);
                }

                float alpha;
                if (t <= node->percentMid)
                {
                    float localT = (node->percentMid > 0.0f) ? t / node->percentMid : 0.0f;
                    alpha = glm::mix(alphaStart, alphaMid, localT);
                }
                else
                {
                    float localT = (1.0f - node->percentMid > 0.0f)
                        ? (t - node->percentMid) / (1.0f - node->percentMid)
                        : 1.0f;
                    alpha = glm::mix(alphaMid, alphaEnd, localT);
                }

                int frameIndex = p.frameIndex;
                int gridX = node->xGrid > 0 ? node->xGrid : 1;
                int gridY = node->yGrid > 0 ? node->yGrid : 1;
                int frameX = frameIndex % gridX;
                int frameY = frameIndex / gridX;
                float texOffsetX = static_cast<float>(frameX) / static_cast<float>(gridX);
                float texOffsetY = static_cast<float>(frameY) / static_cast<float>(gridY);
                m_ParticleShader.setVec2("uTexOffset", glm::vec2(texOffsetX, texOffsetY));

                m_ParticleShader.setVec3("uParticlePos", p.position);
                m_ParticleShader.setFloat("uParticleSize", size);
                m_ParticleShader.setFloat("uParticleRotation", p.rotation);
                m_ParticleShader.setVec4("uParticleColor", glm::vec4(color, alpha));
                m_ParticleShader.setVec3("uParticleVelocity", p.velocity);
                m_ParticleShader.setVec3("uParticleDirection", p.initialDirection);
                m_ParticleShader.setBool("uHasChainTarget", hasChainTarget);
                m_ParticleShader.setVec3("uPrevParticlePos", prevPos);

                gl()->glDrawArrays(GL_TRIANGLES, 0, 6);
            };

            if (renderMode == 1)
            {
                std::vector<const Particle*> chainParticles;
                for (const auto& p : emitter->getParticles())
                {
                    if (p.alive)
                    {
                        chainParticles.push_back(&p);
                    }
                }

                std::sort(chainParticles.begin(), chainParticles.end(),
                          [](const Particle* a, const Particle* b)
                          { return a->spawnIndex < b->spawnIndex; });

                for (size_t i = 0; i < chainParticles.size(); ++i)
                {
                    const Particle* p = chainParticles[i];
                    if (i > 0)
                    {
                        const Particle* prev = chainParticles[i - 1];
                        renderParticle(*p, true, prev->position);
                    }
                    else
                    {
                        renderParticle(*p, false, glm::vec3(0.0f));
                    }
                }
            }
            else
            {
                for (const auto& p : emitter->getParticles())
                {
                    if (!p.alive)
                        continue;
                    renderParticle(p, false, glm::vec3(0.0f));
                }
            }

            if (texture)
            {
                texture->unbind();
            }
        }

        gl()->glBindVertexArray(0);

        gl()->glDepthMask(depthMask);
        gl()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

} // namespace nwn
