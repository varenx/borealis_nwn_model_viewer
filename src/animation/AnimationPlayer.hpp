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
#include <functional>
#include <string>
#include <unordered_map>
#include "KeyframeInterpolator.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    using mdl::Animation;
    using mdl::createTransformMatrix;
    using mdl::EmitterNode;
    using mdl::Model;
    using mdl::Node;
    using mdl::NodeType;

    // Use types from the library
    using mdl::PlaybackState;
    using EventCallback = std::function<void(const std::string& eventName)>;
    using mdl::AnimatedEmitterProperties;
    using mdl::AnimatedTransform;

    // Animation player that handles playback and interpolation
    class AnimationPlayer
    {
    public:
        AnimationPlayer() = default;
        ~AnimationPlayer() = default;

        // Set the model to animate
        void setModel(Model* model);

        // Animation control
        void play(const std::string& animName);
        void play(size_t animIndex);
        void stop();
        void pause();
        void resume();

        // Update animation state (call each frame with delta time)
        void update(float deltaTime);

        // Playback settings
        void setLooping(bool loop)
        {
            m_Looping = loop;
        }
        bool isLooping() const
        {
            return m_Looping;
        }

        void setPlaybackSpeed(float speed)
        {
            m_PlaybackSpeed = speed;
        }
        float getPlaybackSpeed() const
        {
            return m_PlaybackSpeed;
        }

        // Current state
        PlaybackState getState() const
        {
            return m_State;
        }
        float getCurrentTime() const
        {
            return m_CurrentTime;
        }
        float getAnimationLength() const;
        const std::string& getCurrentAnimationName() const
        {
            return m_CurrentAnimName;
        }
        size_t getCurrentAnimationIndex() const
        {
            return m_CurrentAnimIndex;
        }

        // Seek to specific time
        void setTime(float time);

        // Get list of available animations
        std::vector<std::string> getAnimationNames() const;
        size_t getAnimationCount() const;

        // Get interpolated transforms for rendering
        const std::unordered_map<std::string, AnimatedTransform>& getAnimatedTransforms() const
        {
            return m_AnimatedTransforms;
        }

        // Get bone matrices for skinned meshes
        const std::unordered_map<std::string, glm::mat4>& getBoneMatrices() const
        {
            return m_BoneMatrices;
        }

        // Get animated emitter properties (returns persistent properties if animation stopped)
        const std::unordered_map<std::string, AnimatedEmitterProperties>&
        getAnimatedEmitterProperties() const
        {
            // If animation is stopped but we have persistent properties, use those
            if (m_State == PlaybackState::Stopped && !m_PersistentEmitterProperties.empty())
            {
                return m_PersistentEmitterProperties;
            }
            return m_AnimatedEmitterProperties;
        }

        // Event callback
        void setEventCallback(EventCallback callback)
        {
            m_EventCallback = std::move(callback);
        }

    private:
        // Apply animation to node tree
        void applyAnimation();
        void applyAnimationToNode(const Node* animNode, const Node* modelNode);

        // Calculate bone matrices for skinned meshes
        void calculateBoneMatrices();
        void calculateBoneMatricesRecursive(const Node* node, const glm::mat4& parentTransform);

        // Fire events that occurred during the time step
        void processEvents(float prevTime, float currTime);

        Model* m_Model{nullptr};
        const Animation* m_CurrentAnim{nullptr};
        std::string m_CurrentAnimName;
        size_t m_CurrentAnimIndex{0};

        PlaybackState m_State{PlaybackState::Stopped};
        float m_CurrentTime{0.0f};
        float m_PlaybackSpeed{1.0f};
        bool m_Looping{true};

        // Cached animated transforms per node name
        std::unordered_map<std::string, AnimatedTransform> m_AnimatedTransforms;

        // Bone matrices for skinned meshes (node name -> world transform)
        std::unordered_map<std::string, glm::mat4> m_BoneMatrices;

        // Animated emitter properties per emitter name
        std::unordered_map<std::string, AnimatedEmitterProperties> m_AnimatedEmitterProperties;

        // Persistent emitter properties (kept after non-looping animation ends)
        std::unordered_map<std::string, AnimatedEmitterProperties> m_PersistentEmitterProperties;

        // Event handling
        EventCallback m_EventCallback;
        size_t m_LastEventIndex{0};
    };

} // namespace nwn
