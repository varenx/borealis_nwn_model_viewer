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
#include "AnimationPlayer.hpp"
#include <algorithm>
#include <functional>
#include <spdlog/spdlog.h>

namespace nwn
{

    void AnimationPlayer::setModel(Model* model)
    {
        m_Model = model;
        stop();
        m_AnimatedTransforms.clear();
        m_BoneMatrices.clear();
        m_AnimatedEmitterProperties.clear();
        m_PersistentEmitterProperties.clear();

        if (m_Model && m_Model->rootNode)
        {
            // Initialize bone matrices with model's base transforms
            calculateBoneMatrices();
        }
    }

    void AnimationPlayer::play(const std::string& animName)
    {
        if (!m_Model)
        {
            return;
        }

        // Find animation by name
        for (size_t i = 0; i < m_Model->animations.size(); ++i)
        {
            if (m_Model->animations[i].name == animName)
            {
                play(i);
                return;
            }
        }

        spdlog::warn("Animation not found: {}", animName);
    }

    void AnimationPlayer::play(size_t animIndex)
    {
        if (!m_Model || animIndex >= m_Model->animations.size())
        {
            return;
        }

        m_CurrentAnimIndex = animIndex;
        m_CurrentAnim = &m_Model->animations[animIndex];
        m_CurrentAnimName = m_CurrentAnim->name;
        m_CurrentTime = 0.0f;
        m_LastEventIndex = 0;
        m_State = PlaybackState::Playing;

        // Clear persistent properties when starting a new animation
        m_PersistentEmitterProperties.clear();

        spdlog::info("Playing animation: {} (length: {}s)", m_CurrentAnimName,
                     m_CurrentAnim->length);

        applyAnimation();
    }

    void AnimationPlayer::stop()
    {
        m_State = PlaybackState::Stopped;
        m_CurrentTime = 0.0f;
        m_LastEventIndex = 0;
        m_CurrentAnim = nullptr;
        m_CurrentAnimName.clear();
        m_AnimatedTransforms.clear();
        m_AnimatedEmitterProperties.clear();
        m_PersistentEmitterProperties.clear();

        // Reset bone matrices to model's base pose
        if (m_Model && m_Model->rootNode)
        {
            calculateBoneMatrices();
        }
    }

    void AnimationPlayer::pause()
    {
        if (m_State == PlaybackState::Playing)
        {
            m_State = PlaybackState::Paused;
        }
    }

    void AnimationPlayer::resume()
    {
        if (m_State == PlaybackState::Paused)
        {
            m_State = PlaybackState::Playing;
        }
    }

    void AnimationPlayer::update(float deltaTime)
    {
        if (m_State != PlaybackState::Playing || !m_CurrentAnim)
        {
            return;
        }

        float prevTime = m_CurrentTime;
        m_CurrentTime += deltaTime * m_PlaybackSpeed;

        // Handle animation end
        if (m_CurrentTime >= m_CurrentAnim->length)
        {
            if (m_Looping)
            {
                // Process events for remaining time
                processEvents(prevTime, m_CurrentAnim->length);

                // Wrap around
                m_CurrentTime = std::fmod(m_CurrentTime, m_CurrentAnim->length);
                m_LastEventIndex = 0;
                prevTime = 0.0f;
            }
            else
            {
                m_CurrentTime = m_CurrentAnim->length;
                m_State = PlaybackState::Stopped;

                // For non-looping animations, preserve emitter properties
                applyAnimation();
                m_PersistentEmitterProperties = m_AnimatedEmitterProperties;
                return;
            }
        }

        // Process events
        processEvents(prevTime, m_CurrentTime);

        // Apply animation
        applyAnimation();
    }

    float AnimationPlayer::getAnimationLength() const
    {
        return m_CurrentAnim ? m_CurrentAnim->length : 0.0f;
    }

    void AnimationPlayer::setTime(float time)
    {
        if (!m_CurrentAnim)
        {
            return;
        }

        m_CurrentTime = std::clamp(time, 0.0f, m_CurrentAnim->length);
        m_LastEventIndex = 0;

        // Find the last event before current time
        for (size_t i = 0; i < m_CurrentAnim->events.size(); ++i)
        {
            if (m_CurrentAnim->events[i].time <= m_CurrentTime)
            {
                m_LastEventIndex = i + 1;
            }
        }

        applyAnimation();
    }

    std::vector<std::string> AnimationPlayer::getAnimationNames() const
    {
        std::vector<std::string> names;
        if (m_Model)
        {
            names.reserve(m_Model->animations.size());
            for (const auto& anim : m_Model->animations)
            {
                names.push_back(anim.name);
            }
        }
        return names;
    }

    size_t AnimationPlayer::getAnimationCount() const
    {
        return m_Model ? m_Model->animations.size() : 0;
    }

    void AnimationPlayer::applyAnimation()
    {
        if (!m_CurrentAnim || !m_CurrentAnim->rootNode || !m_Model || !m_Model->rootNode)
        {
            return;
        }

        // Clear previous transforms and emitter properties
        m_AnimatedTransforms.clear();
        m_AnimatedEmitterProperties.clear();

        // Apply animation to matching nodes
        applyAnimationToNode(m_CurrentAnim->rootNode.get(), m_Model->rootNode.get());

        // Recalculate bone matrices with animated transforms
        calculateBoneMatrices();
    }

    void AnimationPlayer::applyAnimationToNode(const Node* animNode, const Node* modelNode)
    {
        if (!animNode)
        {
            return;
        }

        if (!modelNode)
        {
            spdlog::warn("Animation node '{}' has no matching model node", animNode->base().name);
            for (const auto& animChild : animNode->base().children)
            {
                applyAnimationToNode(animChild.get(), nullptr);
            }
            return;
        }

        const auto& animBase = animNode->base();
        const auto& modelBase = modelNode->base();

        // Check if this node has animation data
        AnimatedTransform transform;
        transform.position = modelBase.position;
        transform.orientation = modelBase.orientation;
        transform.scale = modelBase.scale;

        // Interpolate position
        if (animBase.positionController && !animBase.positionController->keyframes.empty())
        {
            transform.position = interpolatePosition(*animBase.positionController, m_CurrentTime);
            transform.hasAnimation = true;
        }

        // Interpolate orientation
        if (animBase.orientationController && !animBase.orientationController->keyframes.empty())
        {
            transform.orientation =
                interpolateOrientation(*animBase.orientationController, m_CurrentTime);
            transform.hasAnimation = true;
        }

        // Interpolate scale
        if (animBase.scaleController && !animBase.scaleController->keyframes.empty())
        {
            transform.scale = interpolateScale(*animBase.scaleController, m_CurrentTime);
            transform.hasAnimation = true;
        }

        // Store transform if animated
        if (transform.hasAnimation)
        {
            m_AnimatedTransforms[animBase.name] = transform;
        }

        // Handle emitter nodes - interpolate emitter-specific controllers
        if (animNode->type == NodeType::Emitter)
        {
            const auto* animEmitter = std::get_if<EmitterNode>(&animNode->data);
            if (animEmitter)
            {
                AnimatedEmitterProperties emitterProps;
                if (modelNode && modelNode->type == NodeType::Emitter)
                {
                    const auto* modelEmitter = std::get_if<EmitterNode>(&modelNode->data);
                    if (modelEmitter)
                    {
                        emitterProps.birthRate = modelEmitter->birthRate;
                        emitterProps.lifeExp = modelEmitter->lifeExp;
                        emitterProps.velocity = modelEmitter->velocity;
                        emitterProps.randVel = modelEmitter->randVel;
                        emitterProps.spread = modelEmitter->spread;
                        emitterProps.particleRot = modelEmitter->particleRot;
                        emitterProps.mass = modelEmitter->mass;
                        emitterProps.colorStart = modelEmitter->colorStart;
                        emitterProps.colorMid = modelEmitter->colorMid;
                        emitterProps.colorEnd = modelEmitter->colorEnd;
                        emitterProps.alphaStart = modelEmitter->alphaStart;
                        emitterProps.alphaMid = modelEmitter->alphaMid;
                        emitterProps.alphaEnd = modelEmitter->alphaEnd;
                        emitterProps.sizeStart = modelEmitter->sizeStart;
                        emitterProps.sizeMid = modelEmitter->sizeMid;
                        emitterProps.sizeEnd = modelEmitter->sizeEnd;
                        emitterProps.xSize = modelEmitter->xSize;
                        emitterProps.ySize = modelEmitter->ySize;
                        emitterProps.fps = modelEmitter->fps;
                        emitterProps.grav = modelEmitter->grav;
                    }
                }

                if (animEmitter->birthRateController &&
                    !animEmitter->birthRateController->keyframes.empty())
                {
                    emitterProps.birthRate =
                        interpolateFloat(*animEmitter->birthRateController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->lifeExpController &&
                    !animEmitter->lifeExpController->keyframes.empty())
                {
                    emitterProps.lifeExp =
                        interpolateFloat(*animEmitter->lifeExpController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->velocityController &&
                    !animEmitter->velocityController->keyframes.empty())
                {
                    emitterProps.velocity =
                        interpolateFloat(*animEmitter->velocityController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->randVelController &&
                    !animEmitter->randVelController->keyframes.empty())
                {
                    emitterProps.randVel =
                        interpolateFloat(*animEmitter->randVelController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->spreadController &&
                    !animEmitter->spreadController->keyframes.empty())
                {
                    emitterProps.spread =
                        interpolateFloat(*animEmitter->spreadController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->particleRotController &&
                    !animEmitter->particleRotController->keyframes.empty())
                {
                    emitterProps.particleRot =
                        interpolateFloat(*animEmitter->particleRotController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->massController && !animEmitter->massController->keyframes.empty())
                {
                    emitterProps.mass =
                        interpolateFloat(*animEmitter->massController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->colorStartController &&
                    !animEmitter->colorStartController->keyframes.empty())
                {
                    emitterProps.colorStart =
                        interpolateColor(*animEmitter->colorStartController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->colorMidController &&
                    !animEmitter->colorMidController->keyframes.empty())
                {
                    emitterProps.colorMid =
                        interpolateColor(*animEmitter->colorMidController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->colorEndController &&
                    !animEmitter->colorEndController->keyframes.empty())
                {
                    emitterProps.colorEnd =
                        interpolateColor(*animEmitter->colorEndController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->alphaStartController &&
                    !animEmitter->alphaStartController->keyframes.empty())
                {
                    emitterProps.alphaStart =
                        interpolateFloat(*animEmitter->alphaStartController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->alphaMidController &&
                    !animEmitter->alphaMidController->keyframes.empty())
                {
                    emitterProps.alphaMid =
                        interpolateFloat(*animEmitter->alphaMidController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->alphaEndController &&
                    !animEmitter->alphaEndController->keyframes.empty())
                {
                    emitterProps.alphaEnd =
                        interpolateFloat(*animEmitter->alphaEndController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->sizeStartController &&
                    !animEmitter->sizeStartController->keyframes.empty())
                {
                    emitterProps.sizeStart =
                        interpolateFloat(*animEmitter->sizeStartController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->sizeMidController &&
                    !animEmitter->sizeMidController->keyframes.empty())
                {
                    emitterProps.sizeMid =
                        interpolateFloat(*animEmitter->sizeMidController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->sizeEndController &&
                    !animEmitter->sizeEndController->keyframes.empty())
                {
                    emitterProps.sizeEnd =
                        interpolateFloat(*animEmitter->sizeEndController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->xSizeController &&
                    !animEmitter->xSizeController->keyframes.empty())
                {
                    emitterProps.xSize =
                        interpolateFloat(*animEmitter->xSizeController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->ySizeController &&
                    !animEmitter->ySizeController->keyframes.empty())
                {
                    emitterProps.ySize =
                        interpolateFloat(*animEmitter->ySizeController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->fpsController && !animEmitter->fpsController->keyframes.empty())
                {
                    emitterProps.fps = interpolateFloat(*animEmitter->fpsController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }
                if (animEmitter->gravController && !animEmitter->gravController->keyframes.empty())
                {
                    emitterProps.grav =
                        interpolateFloat(*animEmitter->gravController, m_CurrentTime);
                    emitterProps.hasAnimation = true;
                }

                if (emitterProps.hasAnimation)
                {
                    m_AnimatedEmitterProperties[animBase.name] = emitterProps;
                }
            }
        }

        // Process children - match by name
        for (const auto& animChild : animBase.children)
        {
            const Node* matchingModelChild = nullptr;

            for (const auto& modelChild : modelBase.children)
            {
                if (modelChild->base().name == animChild->base().name)
                {
                    matchingModelChild = modelChild.get();
                    break;
                }
            }

            if (matchingModelChild)
            {
                applyAnimationToNode(animChild.get(), matchingModelChild);
            }
            else
            {
                applyAnimationToNode(animChild.get(), nullptr);
            }
        }
    }

    void AnimationPlayer::calculateBoneMatrices()
    {
        m_BoneMatrices.clear();

        if (m_Model && m_Model->rootNode)
        {
            calculateBoneMatricesRecursive(m_Model->rootNode.get(), glm::mat4(1.0f));
        }
    }

    void AnimationPlayer::calculateBoneMatricesRecursive(const Node* node,
                                                         const glm::mat4& parentTransform)
    {
        if (!node)
        {
            return;
        }

        const auto& base = node->base();

        // Get transform - use animated if available, otherwise use base
        glm::vec3 position = base.position;
        glm::quat orientation = base.orientation;
        float scale = base.scale;

        auto it = m_AnimatedTransforms.find(base.name);
        if (it != m_AnimatedTransforms.end())
        {
            position = it->second.position;
            orientation = it->second.orientation;
            scale = it->second.scale;
        }

        // Calculate local transform
        glm::mat4 localTransform = createTransformMatrix(position, orientation, scale);

        // Calculate world transform
        glm::mat4 worldTransform = parentTransform * localTransform;

        // Store bone matrix
        m_BoneMatrices[base.name] = worldTransform;

        // Process children
        for (const auto& child : base.children)
        {
            calculateBoneMatricesRecursive(child.get(), worldTransform);
        }
    }

    void AnimationPlayer::processEvents(float prevTime, float currTime)
    {
        if (!m_CurrentAnim || !m_EventCallback)
        {
            return;
        }

        // Fire events that occurred in the time range
        for (size_t i = m_LastEventIndex; i < m_CurrentAnim->events.size(); ++i)
        {
            const auto& event = m_CurrentAnim->events[i];
            if (event.time > prevTime && event.time <= currTime)
            {
                m_EventCallback(event.name);
                m_LastEventIndex = i + 1;
            }
            else if (event.time > currTime)
            {
                break;
            }
        }
    }

} // namespace nwn
