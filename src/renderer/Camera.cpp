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
#include "Camera.hpp"
#include <algorithm>
#include <cmath>

namespace nwn
{

    Camera::Camera()
    {
        reset();
    }

    void Camera::setPerspective(float fovDegrees, float aspectRatio, float nearPlane,
                                float farPlane)
    {
        m_Fov = fovDegrees;
        m_AspectRatio = aspectRatio;
        m_NearPlane = nearPlane;
        m_FarPlane = farPlane;
        m_ProjDirty = true;
    }

    void Camera::rotate(float deltaX, float deltaY)
    {
        m_Yaw -= deltaX * m_RotationSensitivity;
        m_Pitch += deltaY * m_RotationSensitivity;

        // Clamp pitch to avoid gimbal lock
        m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);

        m_ViewDirty = true;
    }

    void Camera::pan(float deltaX, float deltaY)
    {
        // Calculate camera right and up vectors (Z-up coordinate system)
        float cosYaw = std::cos(m_Yaw);
        float sinYaw = std::sin(m_Yaw);
        float cosPitch = std::cos(m_Pitch);
        float sinPitch = std::sin(m_Pitch);

        // Right vector is perpendicular to view direction in XY plane
        glm::vec3 right(cosYaw, sinYaw, 0.0f);
        // Up vector - for Z-up, combine pitch rotation with Z axis
        glm::vec3 up(-sinYaw * sinPitch, cosYaw * sinPitch, cosPitch);

        // Pan in screen space
        float panScale = m_Distance * m_PanSensitivity;
        m_Target -= right * deltaX * panScale;
        m_Target += up * deltaY * panScale;

        m_ViewDirty = true;
    }

    void Camera::zoom(float delta)
    {
        m_Distance *= (1.0f - delta * m_ZoomSensitivity);
        m_Distance = std::max(0.1f, m_Distance);
        m_ViewDirty = true;
    }

    void Camera::reset()
    {
        m_Target = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Distance = 5.0f;
        m_Yaw = 0.0f;
        m_Pitch = 0.3f;
        m_ViewDirty = true;
    }

    void Camera::focusOnBounds(const glm::vec3& min, const glm::vec3& max)
    {
        // Calculate center and size
        m_Target = (min + max) * 0.5f;
        glm::vec3 size = max - min;
        float maxDim = std::max({size.x, size.y, size.z});

        // Set distance to see the whole object
        float fovRad = glm::radians(m_Fov);
        m_Distance = maxDim / (2.0f * std::tan(fovRad / 2.0f)) * 1.5f;
        m_Distance = std::max(1.0f, m_Distance);

        m_ViewDirty = true;
    }

    glm::mat4 Camera::getViewMatrix() const
    {
        if (m_ViewDirty)
        {
            updateViewMatrix();
        }
        return m_ViewMatrix;
    }

    glm::mat4 Camera::getProjectionMatrix() const
    {
        if (m_ProjDirty)
        {
            m_ProjectionMatrix =
                glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
            m_ProjDirty = false;
        }
        return m_ProjectionMatrix;
    }

    glm::vec3 Camera::getPosition() const
    {
        float cosYaw = std::cos(m_Yaw);
        float sinYaw = std::sin(m_Yaw);
        float cosPitch = std::cos(m_Pitch);
        float sinPitch = std::sin(m_Pitch);

        // Z-up coordinate system: yaw rotates around Z, pitch tilts up from XY plane
        glm::vec3 direction(-cosPitch * sinYaw, cosPitch * cosYaw, sinPitch);

        return m_Target + direction * m_Distance;
    }

    void Camera::updateViewMatrix() const
    {
        glm::vec3 position = getPosition();
        // Z-up: use (0, 0, 1) as up vector
        m_ViewMatrix = glm::lookAt(position, m_Target, glm::vec3(0.0f, 0.0f, 1.0f));
        m_ViewDirty = false;
    }

} // namespace nwn
