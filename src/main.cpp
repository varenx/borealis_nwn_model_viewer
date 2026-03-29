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

#include <QApplication>
#include <QSurfaceFormat>
#include <spdlog/spdlog.h>

#include "ui/MainWindow.hpp"

int main(int argc, char* argv[])
{
    // Set up logging
    spdlog::set_level(spdlog::level::info);
    spdlog::info("NWN Model Viewer starting...");

    // Set default OpenGL format
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName("Borealis NWN Model Viewer");
    app.setOrganizationName("Borealis NWN Toolset");
    app.setApplicationVersion("1.0.0");

    nwn::MainWindow mainWindow;
    mainWindow.show();

    // Check if a file was passed on command line
    if (argc > 1)
    {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        mainWindow.loadFile(filePath);
    }

    spdlog::info("Application started");

    return app.exec();
}
