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

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QWidget>

#include "animation/AnimationPlayer.hpp"

namespace nwn
{

    class AnimationControls : public QWidget
    {
        Q_OBJECT

    public:
        explicit AnimationControls(QWidget* parent = nullptr);

        // Connect to animation player
        void setAnimationPlayer(AnimationPlayer* player);

        // Update UI from player state
        void refresh();

    signals:
        void animationChanged(int index);
        void playbackStateChanged(bool playing);

    public slots:
        void onAnimationPlayerUpdate();

    private slots:
        void onAnimationSelected(int index);
        void onPlayPauseClicked();
        void onStopClicked();
        void onSliderMoved(int value);
        void onSliderPressed();
        void onSliderReleased();
        void onLoopToggled(bool checked);
        void onSpeedChanged(double value);

    private:
        void updatePlayPauseButton();
        void populateAnimationList();

        AnimationPlayer* m_Player{nullptr};
        bool m_SliderDragging{false};

        QComboBox* m_AnimCombo;
        QPushButton* m_PlayPauseBtn;
        QPushButton* m_StopBtn;
        QSlider* m_TimeSlider;
        QLabel* m_TimeLabel;
        QCheckBox* m_LoopCheck;
        QDoubleSpinBox* m_SpeedSpin;
    };

} // namespace nwn
