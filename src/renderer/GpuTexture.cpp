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
#include "GpuTexture.hpp"
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>

namespace nwn
{

    using mdl::TextureData;

    static QOpenGLFunctions_3_3_Core* gl()
    {
        static QOpenGLFunctions_3_3_Core* funcs = nullptr;
        if (!funcs)
        {
            funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(
                QOpenGLContext::currentContext());
        }
        return funcs;
    }

    GpuTexture::~GpuTexture()
    {
        if (m_TextureId != 0)
        {
            gl()->glDeleteTextures(1, &m_TextureId);
        }
    }

    GpuTexture::GpuTexture(GpuTexture&& other) noexcept :
        m_TextureId(other.m_TextureId), m_Width(other.m_Width), m_Height(other.m_Height)
    {
        other.m_TextureId = 0;
    }

    GpuTexture& GpuTexture::operator=(GpuTexture&& other) noexcept
    {
        if (this != &other)
        {
            if (m_TextureId != 0)
            {
                gl()->glDeleteTextures(1, &m_TextureId);
            }
            m_TextureId = other.m_TextureId;
            m_Width = other.m_Width;
            m_Height = other.m_Height;
            other.m_TextureId = 0;
        }
        return *this;
    }

    void GpuTexture::upload(const TextureData& texture)
    {
        if (m_TextureId != 0)
        {
            gl()->glDeleteTextures(1, &m_TextureId);
        }

        gl()->glGenTextures(1, &m_TextureId);
        gl()->glBindTexture(GL_TEXTURE_2D, m_TextureId);

        // Set texture parameters
        gl()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gl()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        gl()->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA,
                           GL_UNSIGNED_BYTE, texture.pixels.data());

        // Generate mipmaps
        gl()->glGenerateMipmap(GL_TEXTURE_2D);

        m_Width = texture.width;
        m_Height = texture.height;

        gl()->glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GpuTexture::bind(int unit) const
    {
        gl()->glActiveTexture(GL_TEXTURE0 + unit);
        gl()->glBindTexture(GL_TEXTURE_2D, m_TextureId);
    }

    void GpuTexture::unbind() const
    {
        gl()->glBindTexture(GL_TEXTURE_2D, 0);
    }

} // namespace nwn
