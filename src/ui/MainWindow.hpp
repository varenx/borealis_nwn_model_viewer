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

#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QTimer>
#include <memory>

#include <borealis/nwn/mdl/Mdl.hpp>
#include "AnimationControls.hpp"
#include "ModelTreeView.hpp"
#include "ModelViewport.hpp"

namespace nwn
{

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        // Load model from file
        bool loadFile(const QString& filePath);

    private slots:
        void onOpenFile();
        void onCloseFile();
        void onExit();
        void onResetCamera();
        void onToggleMesh(bool checked);
        void onToggleWireframe(bool checked);
        void onShowSourceCode();
        void onAbout();
        void onModelLoaded();
        void onFrameRendered(float fps);
        void updateAnimationControls();

    private:
        void setupUi();
        void setupMenus();
        void setupStatusBar();
        void updateWindowTitle(const QString& fileName = QString());

        // UI Components
        QSplitter* m_MainSplitter;
        QSplitter* m_LeftSplitter;
        ModelViewport* m_Viewport;
        ModelTreeView* m_TreeView;
        AnimationControls* m_AnimControls;

        // Status bar labels
        QLabel* m_FpsLabel;
        QLabel* m_StatusLabel;

        // Animation update timer
        QTimer m_AnimUpdateTimer;

        // Current file info
        QString m_CurrentFilePath;

        // Menu actions that need to be enabled/disabled
        QAction* m_ShowSourceAction;
    };

} // namespace nwn
