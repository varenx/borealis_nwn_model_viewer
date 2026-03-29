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
#include "MainWindow.hpp"
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>
#include <borealis/nwn/mdl/Writer.hpp>
#include <spdlog/spdlog.h>
#include "MdlSourceViewer.hpp"

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
    {
        setupUi();
        setupMenus();
        setupStatusBar();

        // Animation controls update timer
        connect(&m_AnimUpdateTimer, &QTimer::timeout, this, &MainWindow::updateAnimationControls);
        m_AnimUpdateTimer.setInterval(50); // 20 fps update

        updateWindowTitle();
        resize(1280, 800);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::setupUi()
    {
        // Create main splitter (horizontal: left panel | viewport)
        m_MainSplitter = new QSplitter(Qt::Horizontal, this);
        setCentralWidget(m_MainSplitter);

        // Left panel (tree view + animation controls)
        auto* leftPanel = new QWidget();
        auto* leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        m_TreeView = new ModelTreeView();
        leftLayout->addWidget(m_TreeView, 1);

        m_AnimControls = new AnimationControls();
        leftLayout->addWidget(m_AnimControls);

        m_MainSplitter->addWidget(leftPanel);

        // Viewport
        m_Viewport = new ModelViewport();
        m_MainSplitter->addWidget(m_Viewport);

        // Set splitter sizes (30% left, 70% right)
        m_MainSplitter->setSizes({300, 980});

        // Connect signals
        connect(m_Viewport, &ModelViewport::modelLoaded, this, &MainWindow::onModelLoaded);
        connect(m_Viewport, &ModelViewport::frameRendered, this, &MainWindow::onFrameRendered);
        connect(m_TreeView, &ModelTreeView::nodeVisibilityChanged, m_Viewport,
                &ModelViewport::setNodeVisible);
    }

    void MainWindow::setupMenus()
    {
        // File menu
        auto* fileMenu = menuBar()->addMenu("&File");

        auto* openAction = fileMenu->addAction("&Open...");
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

        auto* closeAction = fileMenu->addAction("&Close");
        closeAction->setShortcut(QKeySequence::Close);
        connect(closeAction, &QAction::triggered, this, &MainWindow::onCloseFile);

        fileMenu->addSeparator();

        auto* exitAction = fileMenu->addAction("E&xit");
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

        // View menu
        auto* viewMenu = menuBar()->addMenu("&View");

        auto* resetCameraAction = viewMenu->addAction("&Reset Camera");
        resetCameraAction->setShortcut(QKeySequence("R"));
        connect(resetCameraAction, &QAction::triggered, this, &MainWindow::onResetCamera);

        viewMenu->addSeparator();

        auto* showMeshAction = viewMenu->addAction("Show &Mesh");
        showMeshAction->setCheckable(true);
        showMeshAction->setChecked(true);
        connect(showMeshAction, &QAction::toggled, this, &MainWindow::onToggleMesh);

        auto* showWireframeAction = viewMenu->addAction("Show &Wireframe");
        showWireframeAction->setCheckable(true);
        showWireframeAction->setChecked(false);
        connect(showWireframeAction, &QAction::toggled, this, &MainWindow::onToggleWireframe);

        viewMenu->addSeparator();

        m_ShowSourceAction = viewMenu->addAction("Show MDL &Source");
        m_ShowSourceAction->setShortcut(QKeySequence("Ctrl+U"));
        m_ShowSourceAction->setEnabled(false); // Enabled when model is loaded
        connect(m_ShowSourceAction, &QAction::triggered, this, &MainWindow::onShowSourceCode);

        // Help menu
        auto* helpMenu = menuBar()->addMenu("&Help");

        auto* aboutAction = helpMenu->addAction("&About");
        connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
    }

    void MainWindow::setupStatusBar()
    {
        m_FpsLabel = new QLabel("FPS: --");
        m_FpsLabel->setFixedWidth(80);
        statusBar()->addPermanentWidget(m_FpsLabel);

        m_StatusLabel = new QLabel("Ready");
        statusBar()->addWidget(m_StatusLabel, 1);
    }

    void MainWindow::updateWindowTitle(const QString& fileName)
    {
        QString title = "Borealis NWN Model Viewer";
        if (!fileName.isEmpty())
        {
            title += " - " + fileName;
        }
        setWindowTitle(title);
    }

    bool MainWindow::loadFile(const QString& filePath)
    {
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists())
        {
            QMessageBox::warning(this, "Error", "File not found: " + filePath);
            return false;
        }

        m_StatusLabel->setText("Loading " + fileInfo.fileName() + "...");

        mdl::MdlLoader loader;
        auto result = loader.loadFromFile(filePath.toStdString());

        if (!result)
        {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to load model:\n%1")
                                      .arg(QString::fromStdString(result.error().message)));
            m_StatusLabel->setText("Ready");
            return false;
        }

        m_CurrentFilePath = filePath;

        // Load into viewport
        m_Viewport->loadModel(std::make_unique<mdl::Model>(std::move(*result)));

        // Update tree view
        m_TreeView->setModel(m_Viewport->getModel());

        // Update animation controls
        m_AnimControls->setAnimationPlayer(m_Viewport->getAnimationPlayer());
        m_AnimUpdateTimer.start();

        // Enable source view action
        m_ShowSourceAction->setEnabled(true);

        updateWindowTitle(fileInfo.fileName());
        m_StatusLabel->setText("Loaded: " + fileInfo.fileName());

        spdlog::info("Loaded model: {}", filePath.toStdString());
        return true;
    }

    void MainWindow::onOpenFile()
    {
        QString filePath = QFileDialog::getOpenFileName(this, "Open Model", QString(),
                                                        "NWN Models (*.mdl);;All Files (*)");

        if (!filePath.isEmpty())
        {
            loadFile(filePath);
        }
    }

    void MainWindow::onCloseFile()
    {
        m_AnimUpdateTimer.stop();
        m_Viewport->clearModel();
        m_TreeView->clear();
        m_AnimControls->setAnimationPlayer(nullptr);
        m_CurrentFilePath.clear();
        m_ShowSourceAction->setEnabled(false);
        updateWindowTitle();
        m_StatusLabel->setText("Ready");
    }

    void MainWindow::onExit()
    {
        close();
    }

    void MainWindow::onResetCamera()
    {
        if (m_Viewport->getModel())
        {
            m_Viewport->focusOnModel();
        }
        else
        {
            m_Viewport->resetCamera();
        }
    }

    void MainWindow::onToggleMesh(bool checked)
    {
        m_Viewport->setShowMesh(checked);
    }

    void MainWindow::onToggleWireframe(bool checked)
    {
        m_Viewport->setShowWireframe(checked);
    }

    void MainWindow::onShowSourceCode()
    {
        const auto* model = m_Viewport->getModel();
        if (!model)
        {
            return;
        }

        mdl::WriteOptions options;
        options.floatPrecision = 6;
        options.includeComments = false;

        auto result = mdl::writeAscii(*model, options);
        if (!result)
        {
            QMessageBox::critical(this, "Error",
                                  QString("Failed to generate MDL source:\n%1")
                                      .arg(QString::fromStdString(result.error().message)));
            return;
        }

        auto* viewer = new MdlSourceViewer(this);
        viewer->setModelName(QString::fromStdString(model->name));
        viewer->setText(QString::fromStdString(*result));
        viewer->setAttribute(Qt::WA_DeleteOnClose);
        viewer->show();
    }

    void MainWindow::onAbout()
    {
        QMessageBox::about(this, "About Borealis NWN Model Viewer",
                           "Borealis NWN Model Viewer\n\n"
                           "A viewer for Neverwinter Nights MDL model files.\n\n"
                           "Supports:\n"
                           "- Binary and ASCII MDL formats\n"
                           "- All mesh types (TriMesh, SkinMesh, AnimMesh, DanglyMesh, AABB)\n"
                           "- Skeletal animation playback\n"
                           "- TGA, DDS, and PLT textures");
    }

    void MainWindow::onModelLoaded()
    {
        spdlog::info("Model loaded into viewport");
    }

    void MainWindow::onFrameRendered(float fps)
    {
        m_FpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    }

    void MainWindow::updateAnimationControls()
    {
        m_AnimControls->onAnimationPlayerUpdate();
    }

} // namespace nwn
