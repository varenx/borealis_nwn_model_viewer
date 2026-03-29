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

#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QTimer>
#include <memory>

#include <borealis/nwn/mdl/Mdl.hpp>
#include "animation/AnimationPlayer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Renderer.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    class ModelViewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
    {
        Q_OBJECT

    public:
        explicit ModelViewport(QWidget* parent = nullptr);
        ~ModelViewport() override;

        // Model operations
        void loadModel(std::unique_ptr<mdl::Model> model);
        void clearModel();

        // Camera controls
        void resetCamera();
        void focusOnModel();

        // Rendering options
        void setShowMesh(bool show);
        void setShowWireframe(bool show);
        void setBackgroundColor(const glm::vec3& color);
        void setNodeVisible(const QString& nodeName, bool visible);

        // Animation control access
        AnimationPlayer* getAnimationPlayer()
        {
            return &m_AnimPlayer;
        }
        const mdl::Model* getModel() const
        {
            return m_Model.get();
        }

    signals:
        void modelLoaded();
        void frameRendered(float fps);

    protected:
        // QOpenGLWidget overrides
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

        // Mouse events
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        void updateAnimation();

        std::unique_ptr<mdl::Model> m_Model;
        Renderer m_Renderer;
        Camera m_Camera;
        AnimationPlayer m_AnimPlayer;

        // Mouse interaction state
        QPoint m_LastMousePos;
        bool m_LeftButtonDown{false};
        bool m_MiddleButtonDown{false};
        bool m_RightButtonDown{false};

        // Animation timer
        QTimer m_AnimTimer;
        QElapsedTimer m_FrameTimer;
        float m_LastFrameTime{0.0f};

        // FPS tracking
        int m_FrameCount{0};
        QElapsedTimer m_FpsTimer;
    };

} // namespace nwn
