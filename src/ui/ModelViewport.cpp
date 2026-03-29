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
#include "ModelViewport.hpp"
#include <QMouseEvent>
#include <QWheelEvent>
#include <spdlog/spdlog.h>

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    ModelViewport::ModelViewport(QWidget* parent) : QOpenGLWidget(parent)
    {
        // Set surface format for OpenGL 3.3 Core
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setDepthBufferSize(24);
        format.setSamples(4);
        setFormat(format);

        // Enable mouse tracking
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);

        // Animation update timer (60 fps target)
        connect(&m_AnimTimer, &QTimer::timeout, this, &ModelViewport::updateAnimation);
        m_AnimTimer.setInterval(16); // ~60 fps

        // Initialize timers
        m_FrameTimer.start();
        m_FpsTimer.start();
    }

    ModelViewport::~ModelViewport()
    {
        makeCurrent();
        m_Renderer.shutdown();
        doneCurrent();
    }

    void ModelViewport::loadModel(std::unique_ptr<mdl::Model> model)
    {
        if (!model)
        {
            return;
        }

        m_Model = std::move(model);

        makeCurrent();
        m_Renderer.prepareModel(*m_Model);
        doneCurrent();

        // Set up animation player
        m_AnimPlayer.setModel(m_Model.get());

        // Auto-play "run" animation for testing
        if (m_AnimPlayer.getAnimationCount() > 0)
        {
            m_AnimPlayer.play("run");
            spdlog::info("Auto-playing 'run' animation");
        }

        // Initialize bone matrices for skinned meshes (before first render)
        const auto& boneMatrices = m_AnimPlayer.getBoneMatrices();
        m_Renderer.setBoneMatrices(&boneMatrices);

        // Initialize animated emitter properties for particles
        const auto& emitterProps = m_AnimPlayer.getAnimatedEmitterProperties();
        m_Renderer.setAnimatedEmitterProperties(&emitterProps);

        // Focus camera on model
        focusOnModel();

        // Start animation timer
        m_AnimTimer.start();

        emit modelLoaded();
        update();
    }

    void ModelViewport::clearModel()
    {
        m_AnimTimer.stop();
        m_AnimPlayer.setModel(nullptr);

        makeCurrent();
        m_Renderer.clearModel();
        doneCurrent();

        m_Model.reset();
        update();
    }

    void ModelViewport::resetCamera()
    {
        m_Camera.reset();
        update();
    }

    void ModelViewport::focusOnModel()
    {
        glm::vec3 min = m_Renderer.getBoundsMin();
        glm::vec3 max = m_Renderer.getBoundsMax();
        m_Camera.focusOnBounds(min, max);
        update();
    }

    void ModelViewport::setShowMesh(bool show)
    {
        m_Renderer.setShowMesh(show);
        update();
    }

    void ModelViewport::setShowWireframe(bool show)
    {
        m_Renderer.setShowWireframe(show);
        update();
    }

    void ModelViewport::setBackgroundColor(const glm::vec3& color)
    {
        m_Renderer.setBackgroundColor(color);
        update();
    }

    void ModelViewport::setNodeVisible(const QString& nodeName, bool visible)
    {
        m_Renderer.setNodeVisible(nodeName.toStdString(), visible);
        update();
    }

    void ModelViewport::initializeGL()
    {
        initializeOpenGLFunctions();

        spdlog::info("OpenGL Version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        spdlog::info("OpenGL Renderer: {}",
                     reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        if (!m_Renderer.initialize())
        {
            spdlog::error("Failed to initialize renderer");
        }
    }

    void ModelViewport::resizeGL(int w, int h)
    {
        m_Camera.setAspectRatio(static_cast<float>(w) / static_cast<float>(h));
        glViewport(0, 0, w, h);
    }

    void ModelViewport::paintGL()
    {
        m_Renderer.render(m_Camera);

        // FPS calculation
        m_FrameCount++;
        if (m_FpsTimer.elapsed() >= 1000)
        {
            float fps = static_cast<float>(m_FrameCount) * 1000.0f /
                static_cast<float>(m_FpsTimer.elapsed());
            emit frameRendered(fps);
            m_FrameCount = 0;
            m_FpsTimer.restart();
        }
    }

    void ModelViewport::mousePressEvent(QMouseEvent* event)
    {
        m_LastMousePos = event->pos();

        if (event->button() == Qt::LeftButton)
        {
            m_LeftButtonDown = true;
        }
        else if (event->button() == Qt::MiddleButton)
        {
            m_MiddleButtonDown = true;
        }
        else if (event->button() == Qt::RightButton)
        {
            m_RightButtonDown = true;
        }

        event->accept();
    }

    void ModelViewport::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            m_LeftButtonDown = false;
        }
        else if (event->button() == Qt::MiddleButton)
        {
            m_MiddleButtonDown = false;
        }
        else if (event->button() == Qt::RightButton)
        {
            m_RightButtonDown = false;
        }

        event->accept();
    }

    void ModelViewport::mouseMoveEvent(QMouseEvent* event)
    {
        QPoint delta = event->pos() - m_LastMousePos;
        m_LastMousePos = event->pos();

        float dx = static_cast<float>(delta.x());
        float dy = static_cast<float>(delta.y());

        if (m_MiddleButtonDown)
        {
            if (event->modifiers() & Qt::ShiftModifier)
            {
                // Shift + MMB: Pan camera (Blender style)
                m_Camera.pan(dx * 0.01f, dy * 0.01f);
            }
            else
            {
                // MMB: Orbit camera (Blender style)
                m_Camera.rotate(dx * 0.5f, dy * 0.5f);
            }
            update();
        }

        event->accept();
    }

    void ModelViewport::wheelEvent(QWheelEvent* event)
    {
        float delta = static_cast<float>(event->angleDelta().y()) / 120.0f;
        m_Camera.zoom(delta);
        update();
        event->accept();
    }

    void ModelViewport::updateAnimation()
    {
        float currentTime = static_cast<float>(m_FrameTimer.elapsed()) / 1000.0f;
        float deltaTime = currentTime - m_LastFrameTime;
        m_LastFrameTime = currentTime;

        // Clamp delta time to prevent large jumps
        deltaTime = std::min(deltaTime, 0.1f);

        m_AnimPlayer.update(deltaTime);

        // Update renderer with animated transforms
        const auto& boneMatrices = m_AnimPlayer.getBoneMatrices();
        m_Renderer.updateNodeTransforms(boneMatrices);
        m_Renderer.setBoneMatrices(&boneMatrices);

        // Update animated emitter properties for particles
        const auto& emitterProps = m_AnimPlayer.getAnimatedEmitterProperties();
        m_Renderer.setAnimatedEmitterProperties(&emitterProps);

        update();
    }

} // namespace nwn
