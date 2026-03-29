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
#include <cmath>
#include <vector>

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    // Find the two keyframes surrounding a given time
    template <typename T>
    std::pair<size_t, size_t> findKeyframePair(const std::vector<mdl::Keyframe<T>>& keyframes,
                                               float time)
    {
        if (keyframes.empty())
        {
            return {0, 0};
        }

        if (keyframes.size() == 1 || time <= keyframes.front().time)
        {
            return {0, 0};
        }

        if (time >= keyframes.back().time)
        {
            size_t last = keyframes.size() - 1;
            return {last, last};
        }

        // Binary search for the correct interval
        size_t low = 0;
        size_t high = keyframes.size() - 1;

        while (high - low > 1)
        {
            size_t mid = (low + high) / 2;
            if (keyframes[mid].time <= time)
            {
                low = mid;
            }
            else
            {
                high = mid;
            }
        }

        return {low, high};
    }

    // Calculate interpolation factor between two keyframes
    template <typename T>
    float calculateFactor(const std::vector<mdl::Keyframe<T>>& keyframes, size_t idx1, size_t idx2,
                          float time)
    {
        if (idx1 == idx2)
        {
            return 0.0f;
        }

        float t1 = keyframes[idx1].time;
        float t2 = keyframes[idx2].time;
        float duration = t2 - t1;

        if (duration <= 0.0f)
        {
            return 0.0f;
        }

        return (time - t1) / duration;
    }

    // Linear interpolation for glm::vec3
    inline glm::vec3 interpolate(const glm::vec3& a, const glm::vec3& b, float t)
    {
        return glm::mix(a, b, t);
    }

    // Linear interpolation for float
    inline float interpolate(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    // Spherical linear interpolation for quaternions
    inline glm::quat interpolate(const glm::quat& a, const glm::quat& b, float t)
    {
        return glm::slerp(a, b, t);
    }

    // Hermite spline interpolation for bezier curves
    inline glm::vec3 hermiteInterpolate(const glm::vec3& p0, const glm::vec3& m0,
                                        const glm::vec3& p1, const glm::vec3& m1, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;

        float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float h10 = t3 - 2.0f * t2 + t;
        float h01 = -2.0f * t3 + 3.0f * t2;
        float h11 = t3 - t2;

        return h00 * p0 + h10 * m0 + h01 * p1 + h11 * m1;
    }

    // Interpolate position from keyframes
    inline glm::vec3 interpolatePosition(const mdl::PositionController& controller, float time)
    {
        const auto& keyframes = controller.keyframes;

        if (keyframes.empty())
        {
            return glm::vec3(0.0f);
        }

        auto [idx1, idx2] = findKeyframePair(keyframes, time);

        if (idx1 == idx2)
        {
            return keyframes[idx1].value;
        }

        float t = calculateFactor(keyframes, idx1, idx2, time);

        if (controller.isBezier)
        {
            return hermiteInterpolate(keyframes[idx1].value, keyframes[idx1].outTangent,
                                      keyframes[idx2].value, keyframes[idx2].inTangent, t);
        }

        return interpolate(keyframes[idx1].value, keyframes[idx2].value, t);
    }

    // Interpolate orientation from keyframes
    inline glm::quat interpolateOrientation(const mdl::OrientationController& controller,
                                            float time)
    {
        const auto& keyframes = controller.keyframes;

        if (keyframes.empty())
        {
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        auto [idx1, idx2] = findKeyframePair(keyframes, time);

        if (idx1 == idx2)
        {
            return keyframes[idx1].value;
        }

        float t = calculateFactor(keyframes, idx1, idx2, time);
        return interpolate(keyframes[idx1].value, keyframes[idx2].value, t);
    }

    // Interpolate scale from keyframes
    inline float interpolateScale(const mdl::ScaleController& controller, float time)
    {
        const auto& keyframes = controller.keyframes;

        if (keyframes.empty())
        {
            return 1.0f;
        }

        auto [idx1, idx2] = findKeyframePair(keyframes, time);

        if (idx1 == idx2)
        {
            return keyframes[idx1].value;
        }

        float t = calculateFactor(keyframes, idx1, idx2, time);
        return interpolate(keyframes[idx1].value, keyframes[idx2].value, t);
    }

    // Interpolate float value from keyframes
    inline float interpolateFloat(const mdl::FloatController& controller, float time)
    {
        const auto& keyframes = controller.keyframes;

        if (keyframes.empty())
        {
            return 0.0f;
        }

        auto [idx1, idx2] = findKeyframePair(keyframes, time);

        if (idx1 == idx2)
        {
            return keyframes[idx1].value;
        }

        float t = calculateFactor(keyframes, idx1, idx2, time);
        return interpolate(keyframes[idx1].value, keyframes[idx2].value, t);
    }

    // Interpolate color from keyframes
    inline glm::vec3 interpolateColor(const mdl::ColorController& controller, float time)
    {
        const auto& keyframes = controller.keyframes;

        if (keyframes.empty())
        {
            return glm::vec3(1.0f);
        }

        auto [idx1, idx2] = findKeyframePair(keyframes, time);

        if (idx1 == idx2)
        {
            return keyframes[idx1].value;
        }

        float t = calculateFactor(keyframes, idx1, idx2, time);
        return interpolate(keyframes[idx1].value, keyframes[idx2].value, t);
    }

} // namespace nwn
