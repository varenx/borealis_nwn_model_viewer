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
#include "AnimationControls.hpp"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace nwn
{

    AnimationControls::AnimationControls(QWidget* parent) : QWidget(parent)
    {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Animation selection
        auto* selectLayout = new QHBoxLayout();
        selectLayout->addWidget(new QLabel("Animation:"));
        m_AnimCombo = new QComboBox();
        m_AnimCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        selectLayout->addWidget(m_AnimCombo);
        mainLayout->addLayout(selectLayout);

        // Playback controls
        auto* controlLayout = new QHBoxLayout();

        m_PlayPauseBtn = new QPushButton("Play");
        m_PlayPauseBtn->setFixedWidth(60);
        controlLayout->addWidget(m_PlayPauseBtn);

        m_StopBtn = new QPushButton("Stop");
        m_StopBtn->setFixedWidth(60);
        controlLayout->addWidget(m_StopBtn);

        controlLayout->addSpacing(10);

        m_LoopCheck = new QCheckBox("Loop");
        m_LoopCheck->setChecked(true);
        controlLayout->addWidget(m_LoopCheck);

        controlLayout->addSpacing(10);

        controlLayout->addWidget(new QLabel("Speed:"));
        m_SpeedSpin = new QDoubleSpinBox();
        m_SpeedSpin->setRange(0.1, 5.0);
        m_SpeedSpin->setSingleStep(0.1);
        m_SpeedSpin->setValue(1.0);
        m_SpeedSpin->setFixedWidth(60);
        controlLayout->addWidget(m_SpeedSpin);

        controlLayout->addStretch();
        mainLayout->addLayout(controlLayout);

        // Time slider
        auto* sliderLayout = new QHBoxLayout();
        m_TimeSlider = new QSlider(Qt::Horizontal);
        m_TimeSlider->setRange(0, 1000);
        m_TimeSlider->setValue(0);
        sliderLayout->addWidget(m_TimeSlider);

        m_TimeLabel = new QLabel("0.00 / 0.00");
        m_TimeLabel->setFixedWidth(100);
        sliderLayout->addWidget(m_TimeLabel);
        mainLayout->addLayout(sliderLayout);

        // Connect signals
        connect(m_AnimCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &AnimationControls::onAnimationSelected);
        connect(m_PlayPauseBtn, &QPushButton::clicked, this,
                &AnimationControls::onPlayPauseClicked);
        connect(m_StopBtn, &QPushButton::clicked, this, &AnimationControls::onStopClicked);
        connect(m_TimeSlider, &QSlider::sliderMoved, this, &AnimationControls::onSliderMoved);
        connect(m_TimeSlider, &QSlider::sliderPressed, this, &AnimationControls::onSliderPressed);
        connect(m_TimeSlider, &QSlider::sliderReleased, this, &AnimationControls::onSliderReleased);
        connect(m_LoopCheck, &QCheckBox::toggled, this, &AnimationControls::onLoopToggled);
        connect(m_SpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                &AnimationControls::onSpeedChanged);

        setEnabled(false);
    }

    void AnimationControls::setAnimationPlayer(AnimationPlayer* player)
    {
        m_Player = player;
        populateAnimationList();
    }

    void AnimationControls::refresh()
    {
        populateAnimationList();
    }

    void AnimationControls::populateAnimationList()
    {
        m_AnimCombo->blockSignals(true);
        m_AnimCombo->clear();

        if (!m_Player)
        {
            setEnabled(false);
            m_AnimCombo->blockSignals(false);
            return;
        }

        auto names = m_Player->getAnimationNames();
        if (names.empty())
        {
            m_AnimCombo->addItem("(No animations)");
            setEnabled(false);
        }
        else
        {
            for (const auto& name : names)
            {
                m_AnimCombo->addItem(QString::fromStdString(name));
            }
            setEnabled(true);
        }

        m_AnimCombo->blockSignals(false);
    }

    void AnimationControls::onAnimationPlayerUpdate()
    {
        if (!m_Player || m_SliderDragging)
        {
            return;
        }

        float currentTime = m_Player->getCurrentTime();
        float length = m_Player->getAnimationLength();

        // Update slider
        if (length > 0.0f)
        {
            int sliderPos = static_cast<int>((currentTime / length) * 1000.0f);
            m_TimeSlider->blockSignals(true);
            m_TimeSlider->setValue(sliderPos);
            m_TimeSlider->blockSignals(false);
        }

        // Update time label
        m_TimeLabel->setText(QString("%1 / %2").arg(currentTime, 0, 'f', 2).arg(length, 0, 'f', 2));

        updatePlayPauseButton();
    }

    void AnimationControls::onAnimationSelected(int index)
    {
        if (!m_Player || index < 0)
        {
            return;
        }

        m_Player->play(static_cast<size_t>(index));
        updatePlayPauseButton();
        emit animationChanged(index);
    }

    void AnimationControls::onPlayPauseClicked()
    {
        if (!m_Player)
        {
            return;
        }

        switch (m_Player->getState())
        {
        case PlaybackState::Stopped:
            if (m_AnimCombo->currentIndex() >= 0)
            {
                m_Player->play(static_cast<size_t>(m_AnimCombo->currentIndex()));
            }
            break;
        case PlaybackState::Playing:
            m_Player->pause();
            break;
        case PlaybackState::Paused:
            m_Player->resume();
            break;
        }

        updatePlayPauseButton();
        emit playbackStateChanged(m_Player->getState() == PlaybackState::Playing);
    }

    void AnimationControls::onStopClicked()
    {
        if (!m_Player)
        {
            return;
        }

        m_Player->stop();
        updatePlayPauseButton();
        emit playbackStateChanged(false);
    }

    void AnimationControls::onSliderMoved(int value)
    {
        if (!m_Player)
        {
            return;
        }

        float length = m_Player->getAnimationLength();
        if (length > 0.0f)
        {
            float time = (static_cast<float>(value) / 1000.0f) * length;
            m_Player->setTime(time);
            m_TimeLabel->setText(QString("%1 / %2").arg(time, 0, 'f', 2).arg(length, 0, 'f', 2));
        }
    }

    void AnimationControls::onSliderPressed()
    {
        m_SliderDragging = true;
    }

    void AnimationControls::onSliderReleased()
    {
        m_SliderDragging = false;
    }

    void AnimationControls::onLoopToggled(bool checked)
    {
        if (m_Player)
        {
            m_Player->setLooping(checked);
        }
    }

    void AnimationControls::onSpeedChanged(double value)
    {
        if (m_Player)
        {
            m_Player->setPlaybackSpeed(static_cast<float>(value));
        }
    }

    void AnimationControls::updatePlayPauseButton()
    {
        if (!m_Player)
        {
            m_PlayPauseBtn->setText("Play");
            return;
        }

        switch (m_Player->getState())
        {
        case PlaybackState::Stopped:
        case PlaybackState::Paused:
            m_PlayPauseBtn->setText("Play");
            break;
        case PlaybackState::Playing:
            m_PlayPauseBtn->setText("Pause");
            break;
        }
    }

} // namespace nwn
