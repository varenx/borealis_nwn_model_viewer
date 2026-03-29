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

namespace nwn
{

    class Camera
    {
    public:
        Camera();

        // Set up perspective projection
        void setPerspective(float fovDegrees, float aspectRatio, float nearPlane, float farPlane);

        // Arcball rotation
        void rotate(float deltaX, float deltaY);

        // Pan camera
        void pan(float deltaX, float deltaY);

        // Zoom camera
        void zoom(float delta);

        // Reset to default view
        void reset();

        // Focus on a bounding box
        void focusOnBounds(const glm::vec3& min, const glm::vec3& max);

        // Get matrices
        glm::mat4 getViewMatrix() const;
        glm::mat4 getProjectionMatrix() const;
        glm::vec3 getPosition() const;

        // Settings
        void setAspectRatio(float aspectRatio)
        {
            m_AspectRatio = aspectRatio;
            m_ProjDirty = true;
        }
        void setRotationSensitivity(float sensitivity)
        {
            m_RotationSensitivity = sensitivity;
        }
        void setPanSensitivity(float sensitivity)
        {
            m_PanSensitivity = sensitivity;
        }
        void setZoomSensitivity(float sensitivity)
        {
            m_ZoomSensitivity = sensitivity;
        }

    private:
        void updateViewMatrix() const;

        // Camera parameters
        glm::vec3 m_Target{0.0f, 0.0f, 0.0f}; // Point the camera looks at
        float m_Distance{5.0f}; // Distance from target
        float m_Yaw{0.0f}; // Horizontal rotation (radians)
        float m_Pitch{0.3f}; // Vertical rotation (radians)

        // Projection parameters
        float m_Fov{45.0f};
        float m_AspectRatio{1.0f};
        float m_NearPlane{0.1f};
        float m_FarPlane{1000.0f};

        // Sensitivity settings
        float m_RotationSensitivity{0.005f};
        float m_PanSensitivity{0.01f};
        float m_ZoomSensitivity{0.1f};

        // Cached matrices
        mutable glm::mat4 m_ViewMatrix{1.0f};
        mutable glm::mat4 m_ProjectionMatrix{1.0f};
        mutable bool m_ViewDirty{true};
        mutable bool m_ProjDirty{true};
    };

} // namespace nwn
